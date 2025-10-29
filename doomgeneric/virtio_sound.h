#ifndef __VIRTIO_SOUND__
#define __VIRTIO_SOUND__

#include  <stdint.h>
#include "doomtype.h"

// Device type for virtio-sound
#define VIRTIO_DEVICE_ID_SOUND   25


enum {
  VIRTIO_SND_VQ_CONTROL = 0,    // control queue is used for sending control messages from the driver to the device
  VIRTIO_SND_VQ_EVENT   = 1,    // event queue is used for sending notifications from the device to the driver
  VIRTIO_SND_VQ_TX      = 2,    // tx queue is used to send PCM frames for output streams
  VIRTIO_SND_VQ_RX      = 3,    // rx queue is used to receive PCM frames from input streams
};


enum { 
    /* jack control request types */ 
    VIRTIO_SND_R_JACK_INFO = 1, 
    VIRTIO_SND_R_JACK_REMAP, 
 
    /* PCM control request types */ 
    VIRTIO_SND_R_PCM_INFO = 0x0100, 
    VIRTIO_SND_R_PCM_SET_PARAMS, 
    VIRTIO_SND_R_PCM_PREPARE, 
    VIRTIO_SND_R_PCM_RELEASE, 
    VIRTIO_SND_R_PCM_START, 
    VIRTIO_SND_R_PCM_STOP, 
 
    /* channel map control request types */ 
    VIRTIO_SND_R_CHMAP_INFO = 0x0200, 
 
    /* control element request types */ 
    VIRTIO_SND_R_CTL_INFO = 0x0300, 
    VIRTIO_SND_R_CTL_ENUM_ITEMS, 
    VIRTIO_SND_R_CTL_READ, 
    VIRTIO_SND_R_CTL_WRITE, 
    VIRTIO_SND_R_CTL_TLV_READ, 
    VIRTIO_SND_R_CTL_TLV_WRITE, 
    VIRTIO_SND_R_CTL_TLV_COMMAND, 
 
    /* jack event types */ 
    VIRTIO_SND_EVT_JACK_CONNECTED = 0x1000, 
    VIRTIO_SND_EVT_JACK_DISCONNECTED, 
    /* PCM event types */ 
    VIRTIO_SND_EVT_PCM_PERIOD_ELAPSED = 0x1100, 
    VIRTIO_SND_EVT_PCM_XRUN, 
    /* control element event types */ 
    VIRTIO_SND_EVT_CTL_NOTIFY = 0x1200, 
 
    /* common status codes */ 
    VIRTIO_SND_S_OK = 0x8000, 
    VIRTIO_SND_S_BAD_MSG, 
    VIRTIO_SND_S_NOT_SUPP, 
    VIRTIO_SND_S_IO_ERR 
}; 


enum {
  VIRTIO_SND_D_OUTPUT = 0,
  VIRTIO_SND_D_INPUT  = 1,
};


/* supported PCM stream features */ 
enum { 
    VIRTIO_SND_PCM_F_SHMEM_HOST = 0, 
    VIRTIO_SND_PCM_F_SHMEM_GUEST, 
    VIRTIO_SND_PCM_F_MSG_POLLING, 
    VIRTIO_SND_PCM_F_EVT_SHMEM_PERIODS, 
    VIRTIO_SND_PCM_F_EVT_XRUNS 
}; 
 
/* supported PCM sample formats */ 
enum { 
    /* analog formats (width / physical width) */ 
    VIRTIO_SND_PCM_FMT_IMA_ADPCM = 0,   /*  4 /  4 bits */ 
    VIRTIO_SND_PCM_FMT_MU_LAW,          /*  8 /  8 bits */ 
    VIRTIO_SND_PCM_FMT_A_LAW,           /*  8 /  8 bits */ 
    VIRTIO_SND_PCM_FMT_S8,              /*  8 /  8 bits */ 
    VIRTIO_SND_PCM_FMT_U8,              /*  8 /  8 bits */ 
    VIRTIO_SND_PCM_FMT_S16,             /* 16 / 16 bits */ 
    VIRTIO_SND_PCM_FMT_U16,             /* 16 / 16 bits */ 
    VIRTIO_SND_PCM_FMT_S18_3,           /* 18 / 24 bits */ 
    VIRTIO_SND_PCM_FMT_U18_3,           /* 18 / 24 bits */ 
    VIRTIO_SND_PCM_FMT_S20_3,           /* 20 / 24 bits */ 
    VIRTIO_SND_PCM_FMT_U20_3,           /* 20 / 24 bits */ 
    VIRTIO_SND_PCM_FMT_S24_3,           /* 24 / 24 bits */ 
    VIRTIO_SND_PCM_FMT_U24_3,           /* 24 / 24 bits */ 
    VIRTIO_SND_PCM_FMT_S20,             /* 20 / 32 bits */ 
    VIRTIO_SND_PCM_FMT_U20,             /* 20 / 32 bits */ 
    VIRTIO_SND_PCM_FMT_S24,             /* 24 / 32 bits */ 
    VIRTIO_SND_PCM_FMT_U24,             /* 24 / 32 bits */ 
    VIRTIO_SND_PCM_FMT_S32,             /* 32 / 32 bits */ 
    VIRTIO_SND_PCM_FMT_U32,             /* 32 / 32 bits */ 
    VIRTIO_SND_PCM_FMT_FLOAT,           /* 32 / 32 bits */ 
    VIRTIO_SND_PCM_FMT_FLOAT64,         /* 64 / 64 bits */ 
    /* digital formats (width / physical width) */ 
    VIRTIO_SND_PCM_FMT_DSD_U8,          /*  8 /  8 bits */ 
    VIRTIO_SND_PCM_FMT_DSD_U16,         /* 16 / 16 bits */ 
    VIRTIO_SND_PCM_FMT_DSD_U32,         /* 32 / 32 bits */ 
    VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME  /* 32 / 32 bits */ 
}; 
 
/* supported PCM frame rates */ 
enum { 
    VIRTIO_SND_PCM_RATE_5512 = 0, 
    VIRTIO_SND_PCM_RATE_8000, 
    VIRTIO_SND_PCM_RATE_11025, 
    VIRTIO_SND_PCM_RATE_16000, 
    VIRTIO_SND_PCM_RATE_22050, 
    VIRTIO_SND_PCM_RATE_32000, 
    VIRTIO_SND_PCM_RATE_44100, 
    VIRTIO_SND_PCM_RATE_48000, 
    VIRTIO_SND_PCM_RATE_64000, 
    VIRTIO_SND_PCM_RATE_88200, 
    VIRTIO_SND_PCM_RATE_96000, 
    VIRTIO_SND_PCM_RATE_176400, 
    VIRTIO_SND_PCM_RATE_192000, 
    VIRTIO_SND_PCM_RATE_384000 
}; 

struct __attribute__((packed)) virtio_snd_hdr {
    uint32_t code;  // item request type (VIRTIO_SND_R_*_INFO)
};

struct __attribute__((packed)) virtio_snd_pcm_hdr {
    struct virtio_snd_hdr hdr;
    uint32_t stream_id;
};

struct __attribute__((packed)) virtio_snd_query_info {
  struct virtio_snd_hdr hdr;
  uint32_t start_id;    // starting identifier for the item
  uint32_t count;   // number of items for which information is requested
  uint32_t size;    // size of the structure containing information for one item
};

struct __attribute__((packed)) virtio_snd_info {
    uint32_t hda_fn_nid;    // function group node identifier
};

struct __attribute__((packed)) virtio_snd_pcm_info {
  struct virtio_snd_info info;
  uint32_t features; // bit map of the supported features
  uint64_t formats; // supported sample format bit map
  uint64_t rates;   // supported frame rate bit map
  uint8_t  direction;   // direction of data flow (VIRTIO_SND_D_*)
  uint8_t  channels_min;    // minimum number of supported channels
  uint8_t  channels_max;    // maximum number of supported channels
  uint8_t  _pad[5];
};

struct __attribute__((packed)) virtio_snd_pcm_set_params {
  struct virtio_snd_pcm_hdr hdr; // code=SET_PARAMS
  uint32_t buffer_bytes;
  uint32_t period_bytes;
  uint32_t features;     // 0 for message-based I/O
  uint8_t  channels;     // e.g., 2
  uint8_t  format;       // e.g., S16
  uint8_t  rate;         // e.g., 48k
  uint8_t  _pad;
};

struct __attribute__((packed)) virtio_snd_pcm_xfer {
  uint32_t stream_id;
};

struct __attribute__((packed)) virtio_snd_pcm_status {
  uint32_t status;        // VIRTIO_SND_S_*
  uint32_t latency_bytes; // optional
};


// event struct (device-writable on event queue)
struct __attribute__((packed)) virtio_snd_event {
  struct virtio_snd_hdr hdr; // .code = VIRTIO_SND_EVT_*
  uint32_t data;
};

int virtio_snd_init(void);
int virtio_snd_start(const uint8_t* audio_pcm, const uint32_t audio_pcm_sz, const int8_t ch, const uint8_t vol, const uint8_t sep);
boolean virtio_snd_channel_is_playing(const int8_t ch);
void virtio_snd_stop(const int8_t ch);
void virtio_snd_update_params(const int8_t ch, const uint8_t vol, const uint8_t sep);
int virtio_snd_update();

#endif