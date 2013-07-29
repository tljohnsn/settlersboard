#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <gd.h>
#include <string.h>

// gcc -o sbc settlersboard.c `pkg-config --libs --cflags gtk+-2.0` -lgd
 
 GdkPixbuf *background_image = NULL;
 GdkPixmap *background = NULL;
 GtkStyle *style = NULL;
 GtkWidget *window;

gdImage *tree, *wheat, *sheep, *ore, *brick, *desert, *ports4, *ports6, *outImage, *bg;
gdImage* numbersImage[13];
int deck4[18] = {1,1,1,1,
						2,2,2,2,
						3,3,3,3,
						4,4,4,
						5,5,5
						};
int deck6[28] = {1,1,1,1,1,1,
						2,2,2,2,2,2,
						3,3,3,3,3,3,
						4,4,4,4,4,
						5,5,5,5,5
						};
int *deck;

int numbersDeck4[] = {2, 3, 3, 4, 4, 5, 5, 6, 6, 8, 8, 9, 9, 10, 10, 11, 11, 12, 7};
int numbersDeck6[] = {2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 7, 7};
int board[9][9];
int hexheight, hexwidth, hexside;

int popNum = 0;
int bgW = 1920;
int bgH = 1080;

static int rand_int(int n) 
{
  int limit = RAND_MAX - RAND_MAX % n;
  int rnd;

  do {
    rnd = rand();    
  } while (rnd >= limit);
  return rnd % n;
}

gdImage *copyHex(gdImage *self, gdImage *source, int dstX, int dstY) {
    int $hexwidth = gdImageSY(source);
    int $hexheight = gdImageSX(source);
    int angle = rand_int(6) * 60;
   gdImageCopyRotated(self, source, dstX + hexwidth / 2, dstY + hexheight/2, 0, 0, hexwidth, hexheight, angle);
}

gdImage *get_a_png(char *filename)
{
	gdImage *im;
	FILE *in;
	in = fopen(filename, "rb");
	im = gdImageCreateFromPng(in);
	fclose(in);
	gdImageAlphaBlending(im, 0);
	gdImageSaveAlpha(im, 1);
	return im;
}

gdImage *get_a_default_png(char *filename)
{
	gdImage *im;
	FILE *in;
	in = fopen(filename, "rb");
	im = gdImageCreateFromPng(in);
	fclose(in);
	return im;
}


gdImage *get_image_from_int(int imageNumber)
{
	switch(imageNumber){
		case 0 : return desert;
		case 1 : return tree;
		case 2 : return wheat;
		case 3 : return sheep;
		case 4 : return ore;
		case 5 : return brick;
	}
}

static int pop (int *numbersDeck)
{
	int retval = numbersDeck[popNum];
	popNum++;
	return retval;
}

GdkPixbuf *load_pixbuf_from_file (const char *filename)
{
    GError *error = NULL;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, &error);

    if (pixbuf == NULL)
    {
        g_print ("Error loading file: %d : %s\n", error->code, error->message);
        g_error_free (error);
        exit (1);
    }
    return pixbuf;
}


void fisher_yates_shuffle(int *array, int n) {
  int i, j, tmp;

  for (i = n - 1; i > 0; i--) {
    j = rand_int(i + 1);
    tmp = array[j];
    array[j] = array[i];
    array[i] = tmp;
  }
}

void generate_board(int numplayers)
{
	g_print("Number of players: %d\n", numplayers);
    // Initialize Board array
    int col;
    int row;
    for (col=0; col<9; col++) 
    {
	  for (row=0; row<9; row++) 
	  {
	    board[col][row] = 0;
	  }
    }

    // Distribute numbers first until we find a valid config
    int validboard = 0;
    int *numbersDeck;
    int numNumbers;
    int numDeck;
    while (validboard == 0) {
	if (numplayers >=5 ) {
	    deck = deck6;
	    numbersDeck = numbersDeck6;
	    numDeck = 28;
	    numNumbers = 30;
	} else {
	    deck = deck4;
	    numbersDeck = numbersDeck4;
	    numDeck = 18;
	    numNumbers = 19;
	}

    fisher_yates_shuffle(deck, numDeck);
    fisher_yates_shuffle(numbersDeck, numNumbers);
    
    validboard = 1;
    popNum = 0;
    //Only fill the proper spaces in the hex grid with numbers
    for (col=1; col<=7; col++) {
	for (row=1; row<=6; row++) {
	    if (numplayers >=5 ) {
		if ( (row == 1 || row == 6 ) && (col >= 2 && col <= 5))  {
		    board[col][row] = pop(numbersDeck);
		}
		if ( (row == 2 ) && (col >= 2 && col <= 6))  {
		    board[col][row] = pop(numbersDeck);
		}
		if ( (row == 3 || row == 4 ) && (col >= 1 && col <= 6))  {
		    board[col][row] = pop(numbersDeck);
		}
		if ( (row == 5 ) && (col >= 1 && col <= 5))  {
		    board[col][row] = pop(numbersDeck);
		}
	    } else {
		if ( (row == 2 || row == 6 ) && (col >= 2 && col <= 4))  {
		    board[col][row] = pop(numbersDeck);
		}
		if ( (row == 3 || row == 5 ) && (col >= 1 && col <= 4))  {
		    board[col][row] = pop(numbersDeck);
		}
		if ( (row == 4 || row == 4 ) && (col >= 1 && col <= 5))  {
		    board[col][row] = pop(numbersDeck);
		}
	    }
	    
	    // Check adjacency on 6's and 8's
	    int currentnum = board[col][row];
	    if ((currentnum == 6 || currentnum == 8) && 
	    	(board[col+1][row] == 6 || board[col+1][row] == 8 ||
	    	 board[col-1][row] == 6 || board[col-1][row] == 8 ||
	    	 board[col][row-1] == 6 || board[col][row-1] == 8 ||
	    	 board[col][row+1] == 6 || board[col][row+1] == 8 ||
	    	 (row % 2 == 0 && (board[col-1][row - 1] == 6 || board[col-1][row - 1] == 8 ||
	    	 	board[col-1][row + 1] == 6 || board[col-1][row + 1] == 8)) ||
	    	 (row % 2 == 1 && (board[col+1][row - 1] == 6 || board[col+1][row - 1] == 8 ||
	    	 	board[col+1][row + 1] == 6 || board[col+1][row + 1] == 8)))) 
	    {
	    	validboard = 0;
	    	break;
	    }
	  }
	  if(validboard == 0){
	  	break;
	  }
	}
  }

//Numbers are now arranged in $board, generate an image and distribute tiles
//print "Generate board image\n";
    //Center the board withing the bg image
    int xoff = 0;
    int yoff = 0;
    int portH = 0;
    int portW = 0;
    int lrshift = 0;

    if (numplayers >=5) 
    {
	  xoff = (bgW - hexwidth * 6.5) / 2;
	  yoff = (bgH - hexheight * 4.5) / 2;
	  if (numplayers == 5) 
		{  
			xoff += lrshift = hexwidth * 2;
		}
	  gdImageCopy(outImage, bg, 0 ,0, 0, 0, bgW, bgH);
	  int portW = gdImageSX(ports6);
	  int portH = gdImageSY(ports6);
	  gdImageCopy(outImage, ports6,  (bgW - portW) /2  - hexwidth / 4 + lrshift, (bgH - portH) /2 , 0, 0, portW, portH);
    } else {
	  xoff = (bgW - hexwidth * 7.5) / 2;
	  yoff = (bgH - hexheight * 5.5) / 2;
	  if (numplayers == 3) 
	  {  
	  	xoff += lrshift = hexwidth * 2; 
	  }
	  gdImageCopy(outImage, bg, 0, 0, 0, 0, bgW, bgH);
	  int portW = gdImageSX(ports4);
	  int portH = gdImageSY(ports4);
	  gdImageCopy(outImage, ports4, (bgW - portW) /2 - hexwidth*1.5 + lrshift, (bgH - portH) /2, 0, 0, portW, portH);
    }
gdImage *tile;
popNum = 0;

for (col=1; col<=7; col++) {
    for (row=1; row<=6; row++) {
	int dstX = 0; 
	int dstY = 0;
	if (board[col][row] != 0 ) {
	    if (row  % 2 == 0) {
		  dstX = xoff + hexwidth * (col - 1);
		  dstY = yoff + hexside * 1.5 * (row -1);
	    } else {
		  dstX = xoff + hexwidth / 2 + hexwidth * (col - 1);
		  dstY = yoff + hexside * 1.5 * (row -1);
	    }
	    
	    tile = desert;
	    
	    if (board[col][row] != 7 ) 
	    { 
		  tile = get_image_from_int(pop(deck));
		  copyHex(outImage, tile, dstX, dstY);
		  int imagenumber = board[col][row];
		  copyHex(outImage, numbersImage[imagenumber], dstX, dstY);
	    } else {
		  copyHex(outImage, tile, dstX, dstY);
	    }	
	    //#print "number col,row  board[col][row]\n";
	}
  }    
}
FILE *file;
file = fopen("test.png","wb");
gdImagePng(outImage, file);
fclose(file);

 return;
}

void display_board()
{
  background_image = load_pixbuf_from_file ("test.png");
  gdk_pixbuf_render_pixmap_and_mask (background_image, &background, NULL, 0);
  style = gtk_style_new ();
  style->bg_pixmap [0] = background;
  gtk_widget_set_style (GTK_WIDGET(window), GTK_STYLE (style));

}

void three_player_button_cb(GtkWidget *widget)
{
  generate_board(3);
  display_board();
}

void four_player_button_cb(GtkWidget *widget)
{
  generate_board(4);
  display_board();
}

void five_player_button_cb(GtkWidget *widget)
{
  generate_board(5);
  display_board();
}

void six_player_button_cb(GtkWidget *widget)
{
  generate_board(6);
  display_board();
}


int main(int argc, char** argv) {

  srand(time(NULL));


  GtkWidget *frame;
  GtkWidget *three_player_button;
  GtkWidget *four_player_button;
  GtkWidget *five_player_button;
  GtkWidget *six_player_button;
  GtkWidget *quit_button;
  
  gtk_init(&argc, &argv);

bg = get_a_png("themes/dan/bg.png");
tree = get_a_png("themes/dan/tree.png");
wheat = get_a_png("themes/dan/wheat.png");
sheep = get_a_png("themes/dan/sheep.png");
ore = get_a_png("themes/dan/rock.png");
brick = get_a_png("themes/dan/brick.png");
desert = get_a_png("themes/dan/desert.png");

int i;
char tmpStr[80];
for (i = 2; i<= 12; i++)
{
    if (i !=7) 
    {    	
    	sprintf(tmpStr, "themes/dan/%d.png", i);
    	numbersImage[i] = get_a_png(tmpStr);
    }
}
numbersImage[1]= get_a_default_png("themes/dan/circle.png");
numbersImage[7]= get_a_default_png("themes/dan/circle.png");

ports4 = get_a_default_png("themes/dan/ports4.png");
ports6 = get_a_default_png("themes/dan/ports6.png");

//Calculate the display ofsets of the hexes loaded above
hexwidth = gdImageSX(brick);
hexheight = gdImageSY(brick);
hexside = hexheight / 2;
//g_print "width: $hexwidth height: $hexheight side:$hexside";

//Start building the board image
outImage = get_a_default_png("themes/dan/bg.png");
gdImageAlphaBlending(outImage, 1);
gdImageSaveAlpha(outImage, 1);

if (1==1) {
//    $outImage=GD::Image->newFromPng("$ENV{HOME}/settlersboard.png");
} else {
//    $outImage->copyResampled($bg, 0, 0, 0 ,0, $bgW, $bgH, $bgW, $bgH);
  background_image = load_pixbuf_from_file ("themes/dan/bg.png");
  gdk_pixbuf_render_pixmap_and_mask (background_image, &background, NULL, 0);
  style = gtk_style_new ();
  style->bg_pixmap [0] = background;

}

  background_image = load_pixbuf_from_file ("themes/dan/bg.png");
  gdk_pixbuf_render_pixmap_and_mask (background_image, &background, NULL, 0);
  style = gtk_style_new ();
  style->bg_pixmap [0] = background;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);
  gtk_window_set_title(GTK_WINDOW(window), "Settlers of Catan Board Generator");
  gtk_widget_set_style (GTK_WIDGET(window), GTK_STYLE (style));

  frame = gtk_fixed_new();
  gtk_container_add(GTK_CONTAINER(window), frame);

  three_player_button = gtk_button_new_with_label("3 Player");
  gtk_widget_set_size_request(three_player_button, 70, 35);
  gtk_fixed_put(GTK_FIXED(frame), three_player_button, 20, 20);

  four_player_button = gtk_button_new_with_label("4 Player");
  gtk_widget_set_size_request(four_player_button, 70, 35);
  gtk_fixed_put(GTK_FIXED(frame), four_player_button, 90, 20);

  five_player_button = gtk_button_new_with_label("5 Player");
  gtk_widget_set_size_request(five_player_button, 70, 35);
  gtk_fixed_put(GTK_FIXED(frame), five_player_button, 160, 20);

  six_player_button = gtk_button_new_with_label("6 Player");
  gtk_widget_set_size_request(six_player_button, 70, 35);
  gtk_fixed_put(GTK_FIXED(frame), six_player_button, 230, 20);

  quit_button = gtk_button_new_with_label("Quit");
  gtk_widget_set_size_request(quit_button, 50, 35);
  gtk_fixed_put(GTK_FIXED(frame), quit_button, 300, 20);

  gtk_widget_show_all(window);

  g_signal_connect(window, "destroy",
      G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect(quit_button, "clicked",
      G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect(three_player_button, "clicked", 
      G_CALLBACK(three_player_button_cb), NULL);

  g_signal_connect(four_player_button, "clicked", 
      G_CALLBACK(four_player_button_cb), NULL);

  g_signal_connect(five_player_button, "clicked", 
      G_CALLBACK(five_player_button_cb), NULL);

  g_signal_connect(six_player_button, "clicked", 
      G_CALLBACK(six_player_button_cb), NULL);

  gtk_main();

  return 0;
}