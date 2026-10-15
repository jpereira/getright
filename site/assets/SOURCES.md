# Image sources

- `getright-logo.png`: original GetRight logo from
  https://getright.com/press/getright-logo-800x800.gif, provided by the user and stored
  as `resources/getright-logo.png` in this repository.
- `getright-windows-status.gif`: historical Windows Download Status screenshot from
  https://www.getright.com/images/download-status.gif.
- `getright-windows-progress.gif`: historical Windows download window from
  https://www.getright.com/images/getright-file-download.gif.
- Historical screenshot context: https://www.getright.com/screens.html.
- `port-light.png`, `port-dark.png`, `port-classic.png`, `port-progress.png`, and
  `port-finished.png`: captures of the compiled GetRight Port v1.0.1 transferring an actual
  source archive over a local HTTP connection and displaying its completion tip.
  Run `make screenshots` in a graphical macOS session to regenerate these images.
  These captures use an isolated temporary queue and do not include personal history.
  The capture tool draws a one-pixel black outline around every image for white documentation
  backgrounds.

- `port-window-download.gif`: `port-download.gif` composited over the `port-light.png` main window.
- `port-download.gif`: a looping recording of a real local HTTP download from start to finish.
  Frames come from the same compiled application and are encoded with ffmpeg. The website
  displays a still capture when the viewer prefers reduced motion.

The GetRight name, original logo, and Windows screenshots belong to Headlight Software
and their respective rights holders. They illustrate the original software and are not
licensed as part of the port's LGPL source code. See THIRD_PARTY_NOTICES.md.
