from TTS.api import TTS
import sys

print("LOADING MODEL")
tts = TTS("tts_models/en/multi-dataset/tortoise-v2")

print("GENERATING AUDIO")
tts.tts_to_file(text=sys.argv[1],
                file_path="result/"+sys.argv[3],
                voice_dir="./voices",
                speaker=sys.argv[2],
                num_autoregressive_samples=1,
                diffusion_iterations=10,
                preset="ultra_fast"
                )
