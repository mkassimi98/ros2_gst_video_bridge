# Dependencies

## Required Runtime Packages

For Ubuntu 22.04 / ROS 2 Humble:

```bash
sudo apt-get update
sudo apt-get install -y \
  gstreamer1.0-tools \
  gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev
```

## Common Optional Packages

Install these when using SRT, additional codecs, or receiver-side decode pipelines:

```bash
sudo apt-get install -y \
  gstreamer1.0-plugins-bad \
  gstreamer1.0-libav
```

## Plugin Checks

```bash
gst-inspect-1.0 appsrc
gst-inspect-1.0 videoconvert
gst-inspect-1.0 srtsink
gst-inspect-1.0 udpsink
gst-inspect-1.0 rtspclientsink
gst-inspect-1.0 av1enc
gst-inspect-1.0 qsvav1enc
gst-inspect-1.0 x265enc
gst-inspect-1.0 x264enc
gst-inspect-1.0 jpegenc
gst-inspect-1.0 av1parse
gst-inspect-1.0 rtpav1pay
```

## Jetson Notes

Jetson deployments commonly use NVIDIA encoder elements:

```bash
gst-inspect-1.0 nvv4l2h264enc
gst-inspect-1.0 nvv4l2h265enc
gst-inspect-1.0 nvv4l2av1enc
gst-inspect-1.0 nvvidconv
```

The bridge keeps a software fallback path available when `codec.name:=auto` selects hardware first
and repeated startup or streaming failures are detected.
