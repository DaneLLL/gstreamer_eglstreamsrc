#include <cstdlib>
#include <gst/gst.h>
#include <gst/gstinfo.h>
#include <glib-unix.h>
#include <dlfcn.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <iostream>
#include <sstream>
#include <thread>

using namespace std;

#define EGL_PRODUCER_LIBRARY "libnveglstreamproducer.so"

/* EGLStream Producer */
typedef gint (*start_eglstream_producer_func)
  (int producer_index, EGLDisplay * display, EGLStreamKHR * stream,
   int width, int height);
typedef gint (*stop_eglstream_producer_func) (int producer_index);

static void *nvEGLStreamProducerLib = NULL;
static start_eglstream_producer_func startProducerFunc = NULL;
static stop_eglstream_producer_func stopProducerFunc = NULL;

#define USE(x) ((void)(x))

static GstPipeline *gst_pipeline = nullptr;
static string launch_string;   

int main(int argc, char** argv) {
    USE(argc);
    USE(argv);

    gst_init (&argc, &argv);

    GMainLoop *main_loop;
    main_loop = g_main_loop_new (NULL, FALSE);
    ostringstream launch_stream;
    int w = 1280;
    int h = 720;
    EGLDisplay display;
    EGLStreamKHR stream;
    int ret = 0;

    launch_stream
    << "nveglstreamsrc name=egl_src ! "
    << "video/x-raw(memory:NVMM), format=I420, width="<< w <<", height="<< h <<", framerate=30/1 ! "
    << "omxh264enc bitrate=2500000 name=video_enc iframeinterval=15 profile=high low-latency=false control-rate=4 ! queue ! "
    << "video/x-h264, stream-format=byte-stream  ! "
//    << "filesink location=test.264 ";
    << "fakesink ";

    launch_string = launch_stream.str();

    g_print("Using launch string: %s\n", launch_string.c_str());

    GError *error = nullptr;
    gst_pipeline  = (GstPipeline*) gst_parse_launch(launch_string.c_str(), &error);

    if (gst_pipeline == nullptr) {
        g_print( "Failed to parse launch: %s\n", error->message);
        return -1;
    }
    if(error) g_error_free(error);

    nvEGLStreamProducerLib = dlopen (EGL_PRODUCER_LIBRARY, RTLD_NOW);
    //  Get the startProduceFunc pointer
    startProducerFunc = (start_eglstream_producer_func)
        dlsym (nvEGLStreamProducerLib, "start_eglstream_producer");
    //  Get the stopProduceFunc pointer
    stopProducerFunc = (stop_eglstream_producer_func)
        dlsym (nvEGLStreamProducerLib, "stop_eglstream_producer");
    /* Start the EGLStream Producer */
    ret = startProducerFunc (0, &display, &stream, w, h);
    g_print("display %p, stream %p\n", display, stream);

    GstElement *videoSource = gst_bin_get_by_name(GST_BIN(gst_pipeline), "egl_src");
    if(!GST_IS_ELEMENT(videoSource)) {
        g_print( "Failed to get source from pipeline \n");
        return -1;
    }
    g_object_set(G_OBJECT(videoSource), "display", display, NULL);
    g_object_set(G_OBJECT(videoSource), "eglstream", stream, NULL);

    gst_element_set_state((GstElement*)gst_pipeline, GST_STATE_PLAYING); 

    sleep(10);
    //g_main_loop_run (main_loop);

    gst_element_set_state((GstElement*)gst_pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(gst_pipeline));
    g_main_loop_unref(main_loop);

    ret = stopProducerFunc (0);
    dlclose(nvEGLStreamProducerLib);

    g_print("last line");
    return 0;
}
