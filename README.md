# nus3audio
Nus3audio container extractor and rebuilder. Use vgmstream or something else to handle idsp/opus after extracted.

Latest build: https://github.com/jam1garner/nus3audio/releases/latest

### Building from source

Just run `make`, or `make debug`.

### Usage

```
nus3audio [options] FILE

  -p  --print    |  Prints info about all the files in the nus3audio archive (Default behavior)
  -e  --extract  |  Extract to 'output' folder, unless another folder is provided
  -w  --write    |  Write generated nus3bank to provided outpath
  -o  --outpath  |  Provide an output folder name for extraction, or output file name for injection
                 |  example: nus3audio -e -o mario_audio mario_sfx.nus3audio
                 |
  -d  --delete   |  Given a index it removes the audio file from the nus3audio file
  -r  --replace  |  Replaces a file in the nus3audio file given an index and a filename
                 |  example: nus3audio mario_sfx.nus3audio -r 0 mario_run.lopus -o mario_sfx_mod.nus3audio
                 |
```
