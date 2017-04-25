# The followin Python script replaces the zoom in/out operators
# with a pan function followed by a zoom.  The range of the zoom
# can be adjusted by this method, too.

def panzoomin():
  T=getcursor()
  pan(T)
  zoom(1.5)

def panzoomout():
  T=getcursor()
  pan(T)
  zoom(0.66667)

unbind("Z", "Zoom In")
unbind("z", "Zoom Out")

bind("Z", "panzoomin")
bind("z", "panzoomout")
