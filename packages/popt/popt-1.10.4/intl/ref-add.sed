/^# Packages using this file: / {
  s/# Packages using this file://
  ta
  :a
  s/ popt / popt /
  tb
  s/ $/ popt /
  :b
  s/^/# Packages using this file:/
}
