import os

env = Environment(
  os.environ
  #HOST_ARCH="x86",
  #TARGET_ARCH="x86",
  #MSVS_ARCH="x86",
)
env.Replace(MSVC_USE_SCRIPT="C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\VC\Auxiliary\\Build\\vcvars32.bat")

bittest = env.Program(
  "ogm-bittest",
  os.path.join("etc", "meta", "check-32-bits.cpp")
)

env.Default(bittest)