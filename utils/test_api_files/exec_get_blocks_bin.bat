@echo on
:loop
echo Iteration.
curl -X POST http://127.0.0.1:11211/getblocks.bin --data-binary @get_blocks.bin --output @get_blocks_response.bin
goto loop




