#include "audio.h"

#include <stdio.h>
#include <string.h>

#include <tremor/ivorbisfile.h>
#include <tremor/ivorbiscodec.h>
#include <3ds.h>

#include "vconv.h"
#include "utilities.h"

// ---- DEFINITIONS ----

static const char *PATH[] = {"romfs:/error.ogg","romfs:/success.ogg"};  // Path to Ogg Vorbis file to play
std::string wholefiles[2];
int fileSampleRates[2];
int fileChannels[2];

void*linearBuffer[3];
ndspWaveBuf waveBufs[3];//【重要】必须确保状态是NDSP_WBUF_DONE时才能操作

static const int THREAD_AFFINITY = -1;           // Execute thread on any core
static const int THREAD_STACK_SZ = 32 * 1024;    // 32kB stack for audio thread

// ---- END DEFINITIONS ----

LightEvent s_event={0};
Thread threadId=nullptr;
bool isPlaying=false;

// ---- HELPER FUNCTIONS ----

// Retrieve strings for libvorbisidec errors
const char *vorbisStrError(int error)
{
    switch(error) {
        case OV_FALSE:
            return "OV_FALSE: A request did not succeed.";
        case OV_HOLE:
            return "OV_HOLE: There was a hole in the page sequence numbers.";
        case OV_EREAD:
            return "OV_EREAD: An underlying read, seek or tell operation "
                   "failed.";
        case OV_EFAULT:
            return "OV_EFAULT: A NULL pointer was passed where none was "
                   "expected, or an internal library error was encountered.";
        case OV_EIMPL:
            return "OV_EIMPL: The stream used a feature which is not "
                   "implemented.";
        case OV_EINVAL:
            return "OV_EINVAL: One or more parameters to a function were "
                   "invalid.";
        case OV_ENOTVORBIS:
            return "OV_ENOTVORBIS: This is not a valid Ogg Vorbis stream.";
        case OV_EBADHEADER:
            return "OV_EBADHEADER: A required header packet was not properly "
                   "formatted.";
        case OV_EVERSION:
            return "OV_EVERSION: The ID header contained an unrecognised "
                   "version number.";
        case OV_EBADPACKET:
            return "OV_EBADPACKET: An audio packet failed to decode properly.";
        case OV_EBADLINK:
            return "OV_EBADLINK: We failed to find data we had seen before or "
                   "the stream was sufficiently corrupt that seeking is "
                   "impossible.";
        case OV_ENOSEEK:
            return "OV_ENOSEEK: An operation that requires seeking was "
                   "requested on an unseekable stream.";
        default:
            return "Unknown error.";
    }
}

// ---- END HELPER FUNCTIONS ----


// NDSP audio frame callback
// This signals the audioThread to decode more things
// once NDSP has played a sound frame, meaning that there should be
// one or more available waveBufs to fill with more data.
void audioCallback(void *const nul_) {
    (void)nul_;  // Unused

    if(!running) { // Quit flag
        return;
    }
    
    LightEvent_Signal(&s_event);
}

// Allocate audio buffer
// 120ms buffer
const size_t SAMPLES_PER_BUF = 44100 * 120 / 1000;
// mono (1) or stereo (2)
const size_t CHANNELS_PER_SAMPLE = 2;
// s16 buffer
const size_t WAVEBUF_BYTESIZE = SAMPLES_PER_BUF * CHANNELS_PER_SAMPLE * sizeof(s16);

void audio_init()
{
    ndspInit();
    // Setup NDSP
    ndspChnReset(0);
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE);
    ndspChnSetRate(0, 44100);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    // Setup LightEvent for synchronisation of audioThread
    LightEvent_Init(&s_event, RESET_ONESHOT);
    // Set the ndsp sound frame callback which signals our audioThread
    ndspSetCallback(audioCallback, NULL);

    for(int i=0;i<2;i++){
        OggVorbis_File vorbisFile;
        FILE *fh=fopen(PATH[i],"rb");
        int error = ov_open(fh, &vorbisFile, NULL, 0);
        if(error) {
            error_msg="Failed to open file: error: "+std::to_string(error)+": "+vorbisStrError(error);
            // Only fclose manually if ov_open failed.
            // If ov_open succeeds, fclose happens in ov_clear.
            fclose(fh);
        }
        vorbis_info *vi = ov_info(&vorbisFile, -1);
        fileSampleRates[i]=vi->rate;
        fileChannels[i]=vi->channels;
        //准备内存
        std::string &wholeaudio=wholefiles[i];
        char readbuf[4096];
        while(true){
            int readbytes=ov_read(&vorbisFile,readbuf,sizeof(readbuf),NULL);
            if(readbytes<=0){
                break;
            }
            wholeaudio.append(readbuf,readbytes);
        }
        ov_clear(&vorbisFile);
    }
    for(int i=0;i<ARRAYSIZE(linearBuffer);i++)
    {
        linearBuffer[i]=linearAlloc(WAVEBUF_BYTESIZE);
    }
}

void audio_release()
{
    ndspChnReset(0);
    // Signal audio thread to quit
    LightEvent_Signal(&s_event);

    // Free the audio thread
    if(threadId!=nullptr){
        threadJoin(threadId, UINT64_MAX);
        threadFree(threadId);
    }

    for(int i=0;i<ARRAYSIZE(linearBuffer);i++)
    {
        linearFree(linearBuffer[i]);
    }

    // Cleanup audio things and de-init platform features
    ndspExit();
}

void subthread_audio_file_play(void *param)
{
    int paramID=*(int*)param;
    
    //播放

    int readbytes=0;
    std::string &wholeaudio=wholefiles[paramID];
    memset(waveBufs,0,sizeof(waveBufs));
    for(int i=0;i<ARRAYSIZE(waveBufs);i++){
        waveBufs[i].data_vaddr = linearBuffer[i];
        waveBufs[i].nsamples   = SAMPLES_PER_BUF;
        waveBufs[i].status     = NDSP_WBUF_DONE;
    }
    while(running&&isPlaying&&readbytes<wholeaudio.size()){
        //查找处于DONE状态的BUF
        for(int i=0;i<ARRAYSIZE(waveBufs);i++)
        {
            if(waveBufs[i].status==NDSP_WBUF_DONE)
            {
                //播放缓冲区
                if(readbytes+WAVEBUF_BYTESIZE>=wholeaudio.size()){
                    int leftsize=wholeaudio.size()-readbytes;
                    memcpy(linearBuffer[i],wholeaudio.data()+readbytes,leftsize);
                    memset(linearBuffer[i]+leftsize,0,WAVEBUF_BYTESIZE-leftsize);
                }else{
                    memcpy(linearBuffer[i],wholeaudio.data()+readbytes,WAVEBUF_BYTESIZE);
                }
                readbytes+=WAVEBUF_BYTESIZE;
                ndspChnWaveBufAdd(0,&waveBufs[i]);
                DSP_FlushDataCache(waveBufs[i].data_vaddr,WAVEBUF_BYTESIZE);
            }
        }
        LightEvent_Wait(&s_event);
    }
    isPlaying=false;
}

void audio_play(int id)
{
    isPlaying=false;
    LightEvent_Signal(&s_event);
    ndspChnWaveBufClear(0);
    if(threadId!=nullptr){
        threadJoin(threadId,U64_MAX);
        threadFree(threadId);
    }
    isPlaying=true;
    // Spawn audio thread

    // Set the thread priority to the main thread's priority ...
    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    // ... then subtract 1, as lower number => higher actual priority ...
    priority -= 1;
    // ... finally, clamp it between 0x18 and 0x3F to guarantee that it's valid.
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;

    // Start the thread, passing the address of our vorbisFile as an argument.
    threadId = threadCreate(subthread_audio_file_play, &id,
                                         THREAD_STACK_SZ, priority,
                                         THREAD_AFFINITY, false);
}
