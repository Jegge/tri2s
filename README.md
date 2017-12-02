
For the hardware, have a look at http://mignonette-game.com/

Tri2s Readme
============

Here comes some information about Tri2s, in question and answer style:   

Q) Is this really Tetris?
A) No, it isn't. Tri2s (speak: Tritris) is a cheap ripoff, even the stones broke during the process and some got
   lost, so that's why there are only two types of stones and they're only made out of three blocks instead of 
   four.

Q) How to build the program?
A) Unzip the file and enter the directory, then type "make"

Q) How to program it?
A) After compiling, type "sudo make program". You may need to specifiy the programmer used in the Makefile. Look out
   for the line which says "AVRDUDE_PROGRAMMER = usbtiny" and set it according your programmer.

Q) How to play?
A) After tri2s got transferred onto your mignonette, you're like to hear the music and see the startup screen, a
   capital T. Hold your mignonette in a way that you can read it without turning your head :)
   Now press any key to continue, the game will start directly. Use the C-button to move the stone to the left,
   the D-button to move it to the right. The A-button rotates the stone counter clockwise.
   If you need a break, press the B-button; this will pause the game until any key got pressed.
   When the game is over a skull image will be shown. Press any key to return to the startup screen.

Q) May I copy and share this game?
A) Sure. It's licensed and released under the Creative Commons CC-by-nc-sa license.

Q) Did you do it all alone?
A) The game itself - yes. But I was building upon the work of others, namely Rolf (who provided the miggl library and
   some sample code from munch and who helped me fixing some obscure error*, that I got rid more by chance than by 
   knowledge) and Mitch (who also contributed to the library and munch and who encouraged me soldering the mignonette
   on the 26C3 workshop in berlin by giving me a "solder sticker").
