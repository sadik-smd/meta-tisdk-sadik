# Disable the flite TTS backend. flite 2.2 fails to link on aarch64 due to
# an upstream bug (undefined reference to `cmu_time_awb_sts`).
# QtTextToSpeech will still be built and available to QGroundControl, but
# will use the null/stub backend at runtime (no audio speech output).
PACKAGECONFIG:remove = "flite"
