import sys
from TTS.api import TTS

# Init TTS with the target model name
tts = TTS(model_name="tts_models/en/ljspeech/tacotron2-DDC", progress_bar=True, gpu=False)
# Run TTS
tts.tts_to_file(text=sys.argv[1], file_path=sys.argv[2])
