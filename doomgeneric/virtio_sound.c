#include <string.h>
#include "uart_serial.h"
#include "virtio.h"
#include "virtio_sound.h"
#include "virt_clint.h"
#include "doomtype.h"

#define AUDIO_RATE  11025u //44100u

// audio index in virtio device list
static int dev_idx = -1;

// Static allocation of virtqueues (aligned and contiguous in physical memory)

//
// Control queue is used for sending control messages from the driver to the device
//
// See https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html#x1-350007
// NB: to reduce complexity description ring has size two: one for msg request end one for response
static struct virtq_desc    snd_ctrl_desc[2]          __attribute__((aligned(16)));
static struct virtq_avail   snd_ctrl_avail            __attribute__((aligned(2)));
static struct virtq_used    snd_ctrl_used             __attribute__((aligned(4)));

// Track next Ctrl used slot
static uint16_t snd_ctrl_used_idx = 0;

//
// Transmission queue is used for sending control messages from the driver to the device
//
static struct virtq_desc    snd_tx_desc[QUEUE_SIZE]   __attribute__((aligned(16)));
static struct virtq_avail   snd_tx_avail            __attribute__((aligned(2)));
static struct virtq_used    snd_tx_used             __attribute__((aligned(4)));

// Track which Tx desc are in use (0=free, 1=in use)
static int8_t snd_tx_desc_in_use[QUEUE_SIZE];

// Track next Tx used slot
static uint16_t snd_tx_used_idx = 0;

static struct virtio_snd_pcm_xfer xfer_hdr = { .stream_id = 0 /*/ Default QEMU VIRT playback_stream_idx*/ }; // single instance 
static struct virtio_snd_pcm_status xfer_st[QUEUE_SIZE];


// utility functions usully part of libc implementation
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

// Send one control message (request/out + response/in [+ optional payloads]) and wait
int ctrl_msg(int dev_idx, const void* out, uint32_t out_len, void* in, uint32_t in_len) {

    // desc 0: out
    snd_ctrl_desc[0].addr = (uint64_t)out;
    snd_ctrl_desc[0].len  = out_len;
    snd_ctrl_desc[0].flags= VIRTQ_DESC_F_NEXT; // device-readable (default) & buffer continues
    snd_ctrl_desc[0].next = 1;

    // desc 1: in
    snd_ctrl_desc[1].addr = (uint64_t)in;
    snd_ctrl_desc[1].len  = in_len;
    snd_ctrl_desc[1].flags= VIRTQ_DESC_F_WRITE; // device-writable
    snd_ctrl_desc[1].next = 0;

    uint16_t idx = snd_ctrl_avail.idx;
    snd_ctrl_avail.ring[idx % QUEUE_SIZE] = 0; // index of first desc
    // ask device not to use interrupt on new events. we'll pool for new events
    snd_ctrl_avail.flags = VIRTQ_AVAIL_F_NO_INTERRUPT;

    wmb();  // Make desc/ring visible first

    // Publish avail.idx
    snd_ctrl_avail.idx++;

    io_wmb();   // Ensure idx update is visible before MMIO notify

    // notify control queue
    virtio_mmio_devices[dev_idx].queueNotify = VIRTIO_SND_VQ_CONTROL;

    kprintf("ctrl_msg: waiting for device response ...\n");
    kprintf("snd_ctrl_used_idx [%d] - snd_ctrl_used.idx [%d]\n", snd_ctrl_used_idx, snd_ctrl_used.idx);
    while (1) {
        if (snd_ctrl_used_idx < snd_ctrl_used.idx) {
            rmb();  // ensure used->ring[slot] loads aren't reordered before idx read
            kprintf("ctrl_msg: response received\n");
            snd_ctrl_used_idx++;
            return 1;
        }
    }
}

void print_sound_device_stats(int dev_idx)
{
    kprintf("Sound Device\n");
    kprintf("deviceId [%x]\n", virtio_mmio_devices[dev_idx].deviceId);
    uint32_t* snd_cfg = (uint32_t*)(virtio_mmio_devices[dev_idx].config);
    kprintf("jacks number: %d\n", snd_cfg[0]);
    kprintf("streams number: %d\n", snd_cfg[1]);
    kprintf("chmaps number: %d\n", snd_cfg[2]);
    kprintf("\n");
}

int get_playback_stream_idx(int dev_idx) {
    // Read the pcm streams field to get the number of streams
    uint32_t* snd_cfg = (uint32_t*)(virtio_mmio_devices[dev_idx].config);
    uint32_t jacks_count   = snd_cfg[0];    // not used
    uint32_t streams_count = snd_cfg[1];
    uint32_t chmaps_count  = snd_cfg[2];    // not used

    // send a control request to query information about the available PCM streams 
    // Query PCM_INFO for streams [0..streams-1]; we expect stream 0 is playback (QEMU default)
    struct virtio_snd_query_info q;
    struct __attribute__((packed)) virtio_snd_query_info_resp
    {
        struct virtio_snd_hdr hdr;
        struct virtio_snd_pcm_info infos[8]; // payload - support up to 8 streams here
    } resp;
    q.hdr.code = VIRTIO_SND_R_PCM_INFO;
    q.start_id = 0; // ???
    q.count = streams_count; // count payload elements
    q.size = sizeof(struct virtio_snd_pcm_info); // size single payload element

    if (ctrl_msg(dev_idx, &q, sizeof(q), &resp, sizeof(resp)) < 0)
        return -4;
    if (resp.hdr.code != VIRTIO_SND_S_OK)
        return -5;
    int playback_idx = -6;
    for (int i=0; i<streams_count ;++i) {
        kprintf("virtio_sound_init: stream [%d] ", i);
        kprintf("features [%d] ", resp.infos[i].features);
        kprintf("formats [%d] ", resp.infos[i].formats);
        kprintf("rates [%d] ", resp.infos[i].rates);
        kprintf("direction [%d] ", resp.infos[i].direction);
        kprintf("channels_min [%d] ", resp.infos[i].channels_min);
        kprintf("channels_max [%d] ", resp.infos[i].channels_max);
        kprintf("\n");
        if (resp.infos[i].direction == VIRTIO_SND_D_OUTPUT)
            playback_idx = i;
    }
    return playback_idx;
}

int setup_playback_stream(int dev_idx, int playback_stream_idx, uint32_t buffer_bytes, uint32_t period_bytes) {
    struct virtio_snd_pcm_set_params q;
    // bzero(&q, sizeof(q));
    memset(&q, 0, sizeof(q));
    struct virtio_snd_hdr resp;
    // bzero(&resp, sizeof(resp));
    memset(&resp, 0, sizeof(resp));

    q.hdr.hdr.code = VIRTIO_SND_R_PCM_SET_PARAMS;
    q.hdr.stream_id= playback_stream_idx;
    q.buffer_bytes = buffer_bytes;
    q.period_bytes = period_bytes;
    q.features     = 0; // message-based transport
    q.channels     = 2; // stereo audio
    q.format       = VIRTIO_SND_PCM_FMT_U8; // unsigned 8-bit PCM
    q.rate         = VIRTIO_SND_PCM_RATE_11025;  // 11025 Hz

    if (ctrl_msg(dev_idx, &q, sizeof(q), &resp, sizeof(resp)) < 0)
        return -12;
    if(resp.code != VIRTIO_SND_S_OK)
        return -13;
    return 0;
}

int prepare_playback_stream(int dev_idx, int playback_stream_idx) {
    struct virtio_snd_pcm_hdr cmd;
    struct virtio_snd_hdr resp;

    cmd.hdr.code = VIRTIO_SND_R_PCM_PREPARE;
    cmd.stream_id = playback_stream_idx;
    if (ctrl_msg(dev_idx, &cmd, sizeof(cmd), &resp, sizeof(resp)) < 0)
        return -14;
    if (resp.code != VIRTIO_SND_S_OK)
        return -15;
    return 0;
}

int start_playback_stream(int dev_idx, int playback_stream_idx) {
    struct virtio_snd_pcm_hdr cmd;
    struct virtio_snd_hdr resp;

    cmd.hdr.code = VIRTIO_SND_R_PCM_START;
    cmd.stream_id = playback_stream_idx;
    if (ctrl_msg(dev_idx, &cmd, sizeof(cmd), &resp, sizeof(resp)) < 0)
        return -16;
    if (resp.code != VIRTIO_SND_S_OK)
        return -17;
    return 0;
}

// Init virtio sound 
int virtio_snd_init(void) {
    // detect sound device
    // i.e. index of 'virtio_mmio_devices[]'
    dev_idx = detect_virtio_device(VIRTIO_DEVICE_ID_SOUND);
    if ( dev_idx < 0 ) {
        kprintf("virtio_sound_init: sound device not found\n");
        return -1;
    }
    //print_device_stats(dev_idx);

    // Reset status & set it as acknowledge driver
    virtio_mmio_devices[dev_idx].status = 0; // reset status
    virtio_mmio_devices[dev_idx].status = VIRTIO_STATUS_ACKNOWLEDGE;
    virtio_mmio_devices[dev_idx].status |= VIRTIO_STATUS_DRIVER;
    //kprintf("1. Virtio device status [%x]\n", virtio_mmio_devices[dev_idx].status);

    // Driver feature negotiation (we request no optional features)
    virtio_mmio_devices[dev_idx].driverFeaturesSel = 1;  // select high 32 bits
    virtio_mmio_devices[dev_idx].driverFeatures = 0;
    virtio_mmio_devices[dev_idx].driverFeaturesSel = 0; // select low 32 bits
    virtio_mmio_devices[dev_idx].driverFeatures = 0; 
    
    // inform feature setting is done
    virtio_mmio_devices[dev_idx].status |= VIRTIO_STATUS_FEATURES_OK;
    //kprintf("2. Virtio device status [%x]\n", virtio_mmio_devices[dev_idx].status);
    // Confirm FEATURES_OK
    if (virtio_mmio_devices[dev_idx].status & VIRTIO_STATUS_FEATURES_OK == 0)
        return -2;

    // Setup control virtqueue (id==0)
    kprintf("virtio_sound_init: SETUP Control Queue\n");
    if (setup_queue(dev_idx, VIRTIO_SND_VQ_CONTROL, snd_ctrl_desc, &snd_ctrl_avail, &snd_ctrl_used) < 0)
        return -3;

    // Setup tx virtqueue (id==2)
    kprintf("virtio_sound_init: SETUP Tx Queue\n");
    if (setup_queue(dev_idx, VIRTIO_SND_VQ_TX, snd_tx_desc, &snd_tx_avail, &snd_tx_used) < 0)
        return -4;

    // detect playback stream
    kprintf("virtio_sound_init: get playback stream\n");
    int playback_stream_idx =  get_playback_stream_idx(dev_idx);
    if (playback_stream_idx < 0)  // error
        return playback_stream_idx;

    // configure playback stream (stereo, 8 bits PCM @ 11025 Hz)
    kprintf("virtio_sound_init: SETUP playback stream\n");
    uint32_t period_bytes = 4096;
    uint32_t buffer_bytes = 2*period_bytes; // spec: must be multiple of period_bytes
    int res = setup_playback_stream(dev_idx, playback_stream_idx, buffer_bytes, period_bytes);
    if (res < 0)
        return res;
    
    // prepare playback stream 
    kprintf("virtio_sound_init: PREPARE playback stream\n");
    res = prepare_playback_stream(dev_idx, playback_stream_idx);
    if (res < 0)
        return res;

    // start playback stream 
    kprintf("virtio_sound_init: START playback stream\n");
    res = start_playback_stream(dev_idx, playback_stream_idx);
    if (res < 0)
        return res;

    // Finally tell device we’re fully up
    kprintf("Notify device driver is ok\n");
    virtio_mmio_devices[dev_idx].status |= VIRTIO_STATUS_DRIVER_OK;

    return dev_idx;
}

// check if device has consumed (finish playing) an audio chunk
// Returns: >0 if a new consumed is read; 0 when no used entry; -1 if error 
int virtio_snd_read_used() {
    if (snd_tx_used_idx < snd_tx_used.idx) {
        rmb(); // ensure used->ring[slot] loads aren't reordered before idx read

        /* kprintf("Tx queue: used id [%d] len [%d]\n",
            snd_tx_used.ring[snd_tx_used_idx % QUEUE_SIZE].id,  // index of desc
            snd_tx_used.ring[snd_tx_used_idx % QUEUE_SIZE].len); // should be sizeof(xfer_hdr) + nbytes + sizeof(xfer_st) */
        
        int desc_idx = snd_tx_used.ring[snd_tx_used_idx % QUEUE_SIZE].id;

        struct virtio_snd_pcm_status* p_xfer_st = (void *)snd_tx_desc[desc_idx+2].addr;
        // mark desc slots as free
        snd_tx_desc_in_use[desc_idx] = snd_tx_desc_in_use[desc_idx+1] = snd_tx_desc_in_use[desc_idx+2] = 0;

        snd_tx_used_idx++;
        return (p_xfer_st->status == VIRTIO_SND_S_OK)? snd_tx_desc[desc_idx+1].len : - p_xfer_st->status;
    }
    return 0; // no used entry
}

// Submit a PCM buffer to TX queue
int virtio_snd_write(int dev_idx, const void* pcm, uint32_t nbytes){

  // Find free desc (3 needed)
  int idx = 0;
  for (; idx<QUEUE_SIZE ; ++idx) {
    if (idx+2 < QUEUE_SIZE &&
        snd_tx_desc_in_use[idx] == 0 && snd_tx_desc_in_use[idx+1] == 0 && snd_tx_desc_in_use[idx+2] == 0)
        break;
  }
  if (idx == QUEUE_SIZE)
      return -1; // no free desc
  // set desc as in use
  snd_tx_desc_in_use[idx] = snd_tx_desc_in_use[idx+1] = snd_tx_desc_in_use[idx+2] = 1;

  // d0: xfer hdr (out)
  snd_tx_desc[idx].addr  = (uint64_t)&xfer_hdr;
  snd_tx_desc[idx].len   = sizeof(xfer_hdr);
  snd_tx_desc[idx].flags = VIRTQ_DESC_F_NEXT; // device-readable
  snd_tx_desc[idx].next  = idx+1;

  // d1: PCM payload (out)
  snd_tx_desc[idx+1].addr  = (uint64_t)pcm;
  snd_tx_desc[idx+1].len   = nbytes;
  snd_tx_desc[idx+1].flags = VIRTQ_DESC_F_NEXT; // device-readable
  snd_tx_desc[idx+1].next  = idx+2;

  // d2: status (in)
  memset(&xfer_st[idx/3], 0, sizeof(struct virtio_snd_pcm_status)); // reset struct
  snd_tx_desc[idx+2].addr  = (uint64_t)&xfer_st[idx/3];
  snd_tx_desc[idx+2].len   = sizeof(struct virtio_snd_pcm_status);
  snd_tx_desc[idx+2].flags = VIRTQ_DESC_F_WRITE; // device-writable
  snd_tx_desc[idx+2].next  = 0;

  snd_tx_avail.ring[snd_tx_avail.idx % QUEUE_SIZE] = idx; // index of first desc

  wmb();    // Make desc/ring visible first

  // Publish avail.idx
  snd_tx_avail.idx++;

  io_wmb(); // Ensure idx update is visible before MMIO notify

  // notify tx queue
  virtio_mmio_devices[dev_idx].queueNotify = VIRTIO_SND_VQ_TX;

  return 0;
}

uint8_t virtio_snd_playing_chunks_count() {
    uint8_t count = 0;
    for (uint8_t i=0; i < QUEUE_SIZE ;++i)
        count += snd_tx_desc_in_use[i];
    // each chunk submission uses 3 desc slots
    return count/3;
}

#define NUM_CHANNELS 8
struct audio_channel {
    boolean in_use;
    const uint8_t* pcm; // address
    uint32_t pcm_sz;  // size in bytes
    uint8_t vol;    // volume [0, 127]
    uint8_t sep;    // stereo separation [0, 254], 0=left only, 127=center, 254=right only
};
static struct audio_channel channels[NUM_CHANNELS];


// Add audio clip to be played immediately
int virtio_snd_start(const uint8_t* audio_pcm, const uint32_t audio_pcm_sz, const int8_t ch, const uint8_t vol, const uint8_t sep) {
    if (ch<0 || ch>=NUM_CHANNELS)   // invalid channel
        return -1;
    channels[ch].pcm = audio_pcm;
    channels[ch].pcm_sz = audio_pcm_sz;
    channels[ch].vol = vol;
    channels[ch].sep = sep;
    channels[ch].in_use = true;
    return 0;
}

void virtio_snd_stop(const int8_t ch) {
    if (ch>=0 && ch<NUM_CHANNELS)
        channels[ch].in_use = false;
}

void virtio_snd_update_params(const int8_t ch, const uint8_t vol, const uint8_t sep) {
    if (ch<0 || ch>=NUM_CHANNELS)   // invalid channel
        return;
    channels[ch].vol = vol;
    channels[ch].sep = sep;
}

// size of chunk buffer
#define NUM_CHUNKS 2
// num_channels * (samples in a second / num chunks per second)
#define CHUNK_SZ (2  * (AUDIO_RATE  / 35))
static uint8_t chunk_buffer[NUM_CHUNKS][CHUNK_SZ];
static uint8_t next_chunk_idx = 0;


boolean virtio_snd_channel_is_playing(const int8_t ch) {
    //kprintf("virtio_snd_channel_is_playing: ch [%d]\n", ch);
    if (ch<0 || ch>=NUM_CHANNELS)   // invalid channel
        return false;
    return channels[ch].in_use;
}

// at least one audio channel is in use
boolean audio_channels_in_use() {
    for (int i=0; i < NUM_CHANNELS ; ++i)
        if (channels[i].in_use)
            return true;
    return false;
}

boolean chunk_buffer_is_full() {
    uint8_t count = virtio_snd_playing_chunks_count();
    if (count==0) kprintf("chunk_buffer_is_full: WARN - audio underrun!!!\n");
    return ( count >= NUM_CHUNKS ) ?true :false;
}

// mix audio with channel val/sep
// adding values computed with : value * (vol/127) * (sep/127)
void mix_channels_audio_chunk(uint8_t* out_chunk) {
    // proper mixing of all in-use channels
    int tmp[CHUNK_SZ];
    // clear temp buffer
    memset(tmp, 0, sizeof(tmp));
    for (int ch=0; ch < NUM_CHANNELS ; ++ch) {
        if (channels[ch].in_use) {
            int sz = min(CHUNK_SZ, channels[ch].pcm_sz);
            sz &= ~1;   // ensure even size (stereo)
            const int l_gain = channels[ch].vol * channels[ch].sep;
            const int r_gain = channels[ch].vol * (254 - channels[ch].sep);
            for (int i=0; i < sz ; i+=2) {
                // simple mixing: sum all channels samples (with volume adjustment and stereo sepration)
                // left channel
                tmp[i+0] += ((int)channels[ch].pcm[i+0] - 128) * l_gain;
                // right channel
                tmp[i+1] += ((int)channels[ch].pcm[i+1] - 128) * r_gain;
            }
            // update audio data in channels
            if (channels[ch].pcm_sz > CHUNK_SZ) {
                channels[ch].pcm += CHUNK_SZ;
                channels[ch].pcm_sz -= CHUNK_SZ;
            } else {
                channels[ch].pcm = NULL;
                channels[ch].pcm_sz = 0;
                channels[ch].in_use = false;
            }
        }
    }
    // clamp mixed samples and write to output chunk
    for (int i=0; i < CHUNK_SZ ; ++i) {
        tmp[i] /= 127 * 127;    // modulation
        tmp[i] = max(tmp[i], -128);
        tmp[i] = min(tmp[i],  127);
        out_chunk[i] = tmp[i] + 128;
    }
}

// invoked periodically (e.g. every 30 ms) to keep audio playing
// Returns: 1 if some audio still playing; 0 if no more audio playing; -1 if error
int virtio_snd_update() {
    int res;
    // check if a previously sent chunk had been consumed
    while ( (res = virtio_snd_read_used()) > 0 ) {}
    if (res < 0) // error
        return res;

    // if at least one playing audio and chunk buffer is not full
    // compute (mix) and send to virtio device one or more chunck to fill the buffer
    while ( audio_channels_in_use() && !chunk_buffer_is_full() ) {
        uint8_t* mixed_chunk = chunk_buffer[next_chunk_idx];
        // update next chunk index
        next_chunk_idx = (next_chunk_idx+1) % NUM_CHUNKS;
        // compute audio chunk mixing properly audio chunks from channels 
        mix_channels_audio_chunk(mixed_chunk);
        // send chunk to virtio sound device
        res = virtio_snd_write(dev_idx, mixed_chunk, CHUNK_SZ);
        if (res < 0) // error
            return res;
    }

    return audio_channels_in_use() ?1 :0;
}