#include <fcntl.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

// gcc -o sbc settlersboard.c `pkg-config --libs --cflags gtk+-2.0` -O3 -ffast-math
 
GdkPixmap *background = NULL;
GtkStyle *style = NULL;
GtkWidget *window, *frame, *progressBar;

GdkPixbuf *treePB, *wheatPB, *sheepPB, *orePB, *brickPB, *desertPB, *ports4PB, *ports6PB, 
			*outImagePB, *bgPB, *numbersImagePB[13];

char returnString[256];
char themeName[1024] = "dan";
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

int numbersDeck4[] = {2, 3, 3, 4, 4, 5, 5, 6, 6, 8, 8, 9, 9, 10, 10, 11, 11, 12, 7};
int numbersDeck6[] = {2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 7, 7};

int *deck;
int board[9][9];
int hexheight, hexwidth, hexside, numplayersGlobal;

int popNum = 0;
int bgW = 1920;
int bgH = 1080;

int hasBuiltBoard = FALSE;

static int rand_int(int n) 
{
  int limit = RAND_MAX - RAND_MAX % n;
  int rnd;

  do {
    rnd = rand();    
  } while (rnd >= limit);
  return rnd % n;
}

// from libfspot/f-pixbuf-utils.c, copies a cairo surface into a GdkPixBuf
GdkPixbuf *pixbuf_from_cairo_surface (cairo_surface_t *source)
{
  gint width = cairo_image_surface_get_width (source);
  gint height = cairo_image_surface_get_height (source);
  GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,TRUE,8,width,
				      height);

  guchar *gdk_pixels = gdk_pixbuf_get_pixels (pixbuf);
  int gdk_rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  
  cairo_surface_t *surface;
  cairo_t *ctx;
  int j;

  surface = cairo_image_surface_create_for_data (gdk_pixels,
						 CAIRO_FORMAT_RGB24,
						 width, height, gdk_rowstride);
  ctx = cairo_create (surface);
  cairo_set_source_surface (ctx, source, 0, 0);
  cairo_paint (ctx);

  for (j = height; j; j--)
    {
      guchar *p = gdk_pixels;
      guchar *end = p + 4 * width;
      guchar tmp;

      while (p < end)
	{
	  tmp = p[0];
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	  p[0] = p[2];
	  p[2] = tmp;
#else	  
	  p[0] = p[1];
	  p[1] = p[2];
	  p[2] = p[3];
	  p[3] = tmp;
#endif
	  p += 4;
	}

      gdk_pixels += gdk_rowstride;
    }

  cairo_destroy (ctx);
  cairo_surface_destroy (surface);
  return pixbuf;
}

//Rotate a pixbuf and composite into another image
void copyHexPB(GdkPixbuf *self, GdkPixbuf *source, int dstX, int dstY) {
	int width = gdk_pixbuf_get_width(source);
	int height = gdk_pixbuf_get_height(source);    
	int angle = rand_int(6) * 60;
	GdkPixbuf *tmpPB;
    cairo_surface_t *srf = cairo_image_surface_create(CAIRO_FORMAT_RGB24,hexwidth,hexheight);
	cairo_t *cr = cairo_create(srf);
	//cairo rotates around the origin, so the object is translated to center over the origin
	cairo_translate (cr, width * 0.5, height * 0.5);
	cairo_rotate(cr,angle * M_PI / 180);
	cairo_translate (cr, -width * 0.5, -height * 0.5);
	gdk_cairo_set_source_pixbuf(cr, source, 0, 0);
	cairo_paint(cr);
	tmpPB = pixbuf_from_cairo_surface(srf);
	cairo_destroy(cr);
	cairo_surface_destroy(srf);
	gdk_pixbuf_composite(tmpPB, self, dstX, dstY, width, height, dstX,dstY,1,1,GDK_INTERP_NEAREST,255);
	g_object_unref(tmpPB);
}

GdkPixbuf *get_image_from_intPB(int imageNumber)
{
	switch(imageNumber){
		case 0 : return desertPB;
		case 1 : return treePB;
		case 2 : return wheatPB;
		case 3 : return sheepPB;
		case 4 : return orePB;
		case 5 : return brickPB;
		}
		return desertPB;
}

static int pop (int *numbersDeck)
{
	int retval = numbersDeck[popNum];
	popNum++;
	return retval;
}

//convenience function for loading pixbufs
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
    g_free(error);
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

void renderImages()
{
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "Initializing Graphics");
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar),0.3);
  gtk_main_iteration_do(FALSE);

    //Numbers are now arranged in $board, generate an image and distribute tiles
    //Center the board withing the bg image
    int xoff = 0;
    int yoff = 0;
    int lrshift = 0;
    int numplayers = numplayersGlobal;
    int col,row;

    if (numplayers >=5) 
    {
	  xoff = (bgW - hexwidth * 9) / 2;
	  yoff = (bgH - hexheight * 4.5) / 2;
	  if (numplayers == 5) 
		{  
			xoff += lrshift = hexwidth * 2;
		}
	  gdk_pixbuf_composite(bgPB, outImagePB, 0, 0, bgW, bgH, 0,0,1,1,GDK_INTERP_NEAREST,255);
	  int portW = gdk_pixbuf_get_width(ports6PB);
	  int portH = gdk_pixbuf_get_height(ports6PB);
	  int portX = (bgW - portW) /2 - hexwidth*1.5 + lrshift;
	  int portY = (bgH - portH) /2;
	  gdk_pixbuf_composite(ports6PB, outImagePB, portX, portY, portW, portH, portX,portY,1,1,GDK_INTERP_NEAREST,255);
    } else {
	  xoff = (bgW - hexwidth * 7.5) / 2;
	  yoff = (bgH - hexheight * 5.5) / 2;
	  if (numplayers == 3) 
	  {  
	  	xoff += lrshift = hexwidth * 2; 
	  }
	  gdk_pixbuf_composite(bgPB, outImagePB, 0, 0, bgW, bgH, 0,0,1,1,GDK_INTERP_NEAREST,255);
	  int portW = gdk_pixbuf_get_width(ports4PB);
	  int portH = gdk_pixbuf_get_height(ports4PB);
	  int portX = (bgW - portW) /2 - hexwidth*1.5 + lrshift;
	  int portY = (bgH - portH) /2;
	  gdk_pixbuf_composite(ports4PB, outImagePB, portX, portY, portW, portH, portX,portY,1,1,GDK_INTERP_NEAREST,255);
    }

  GdkPixbuf *tilePB;
  popNum = 0;
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "Reticulating Splines");
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar),0.5);
  gtk_main_iteration_do(FALSE);

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
	    
	    tilePB = desertPB;
	    
	    if (board[col][row] != 7 ) 
	    { 
		  tilePB = get_image_from_intPB(pop(deck));
		  copyHexPB(outImagePB, tilePB, dstX, dstY);
		  int imagenumber = board[col][row];
		  copyHexPB(outImagePB, numbersImagePB[imagenumber], dstX, dstY);
	    } else {
		  copyHexPB(outImagePB, tilePB, dstX, dstY);
	    }	
	    //#print "number col,row  board[col][row]\n";
	  }
    }    
  }
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar),0.9);
  gtk_main_iteration_do(FALSE);
}

void generate_board(int numplayers)
{
 	numplayersGlobal = numplayers;
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
    while (!validboard) {

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
  hasBuiltBoard = TRUE;
  renderImages();
}

void display_board()
{
  gdk_pixbuf_render_pixmap_and_mask (outImagePB, &background, NULL, 0);
  style = gtk_style_new ();
  style->bg_pixmap [0] = background;
  gtk_widget_set_style (GTK_WIDGET(window), GTK_STYLE (style));
}

void save_board()
{
  gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progressBar), "Saving image");
  gtk_main_iteration_do(FALSE);
  gdk_pixbuf_save(outImagePB, "savedboard.png","png",NULL,NULL);
}

void three_player_button_cb(GtkWidget *widget)
{
  gtk_widget_show(progressBar);
  gtk_main_iteration_do(FALSE);
  generate_board(3);
  display_board();
  save_board();
  gtk_widget_hide(progressBar);
}

void four_player_button_cb(GtkWidget *widget)
{
  gtk_widget_show(progressBar);
  gtk_main_iteration_do(FALSE);
  generate_board(4);
  display_board();
  save_board();
  gtk_widget_hide(progressBar);
}

void five_player_button_cb(GtkWidget *widget)
{
  gtk_widget_show(progressBar);
  gtk_main_iteration_do(FALSE);
  generate_board(5);
  display_board();
  save_board();  
  gtk_widget_hide(progressBar);
}

void six_player_button_cb(GtkWidget *widget)
{
  gtk_widget_show(progressBar);
  gtk_main_iteration_do(FALSE);
  generate_board(6);
  display_board();
  save_board();
  gtk_widget_hide(progressBar);
}

char* themed_file_path(char *theme, char *filename)
{
	sprintf(returnString, "themes/%s/%s",theme,filename);
	return returnString;
}

void load_images(char *theme)
{	
	bgPB = load_pixbuf_from_file(themed_file_path(theme,"bg.png"));
	treePB = load_pixbuf_from_file(themed_file_path(theme,"tree.png"));
	wheatPB = load_pixbuf_from_file(themed_file_path(theme,"wheat.png"));
	sheepPB = load_pixbuf_from_file(themed_file_path(theme,"sheep.png"));
	orePB = load_pixbuf_from_file(themed_file_path(theme,"rock.png"));
	brickPB = load_pixbuf_from_file(themed_file_path(theme,"brick.png"));
	desertPB = load_pixbuf_from_file(themed_file_path(theme,"desert.png"));
	int i;
	char tmpStr[80];

	for (i = 2; i<= 12; i++)
	{
	    if (i !=7) 
	    {    	
	    	sprintf(tmpStr, "themes/%s/%d.png",theme, i);
	    	numbersImagePB[i] = load_pixbuf_from_file(tmpStr);
	    }
	}
	numbersImagePB[1]= load_pixbuf_from_file(themed_file_path(theme,"circle.png"));
	numbersImagePB[7]= load_pixbuf_from_file(themed_file_path(theme,"circle.png"));

	ports4PB = load_pixbuf_from_file(themed_file_path(theme,"ports4.png"));
	ports6PB = load_pixbuf_from_file(themed_file_path(theme,"ports6.png"));

	//Calculate the display offsets of the hexes loaded above
	hexwidth = gdk_pixbuf_get_width(brickPB);
	hexheight = gdk_pixbuf_get_height(brickPB);
	hexside = hexheight / 2;
}

void free_images()
{
	g_object_unref(brickPB);
	g_object_unref(treePB);
	g_object_unref(wheatPB);
	g_object_unref(sheepPB);
	g_object_unref(orePB);
	g_object_unref(bgPB);
	g_object_unref(desertPB);
	g_object_unref(ports4PB);
	g_object_unref(ports6PB);

	int i;
	for(i=1;i<=12;i++)
	{
		g_object_unref(numbersImagePB[i]);
	}
}

void cb_changed(GtkComboBox *combo, gpointer data)
{

	gchar* string = gtk_combo_box_get_active_text( combo );
	strcpy(themeName, string);
	g_free(string);
	if(hasBuiltBoard) {
	    gtk_widget_show(progressBar);
	    gtk_main_iteration_do(FALSE);
	 	free_images();
		load_images(themeName);
		renderImages();
		display_board();
		gtk_widget_hide(progressBar);
	}
}

void add_theme_dirs(GtkWidget* comboBox)
{
    struct dirent* dent;
    DIR* srcdir = opendir("themes");

    if (srcdir == NULL)
    {
    	perror("opendir");
    	return;
    }

    while((dent = readdir(srcdir)) != NULL)
    {
    	struct stat st;

    	if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
    		continue;

    	if (fstatat(dirfd(srcdir), dent->d_name, &st,0) < 0)
    	{
    		perror(dent->d_name);
    		continue;
    	}

    	if (S_ISDIR(st.st_mode))
    		gtk_combo_box_append_text( GTK_COMBO_BOX( comboBox ), dent->d_name);
    }
    closedir(srcdir);
}

int main(int argc, char** argv) {

  srand(time(NULL));

  GtkWidget *three_player_button, *four_player_button,
  *five_player_button, *six_player_button, *quit_button,
  *themeSelector;
    
  gtk_init(&argc, &argv);

  if(fopen("savedboard.png","r"))
  {
    outImagePB = load_pixbuf_from_file("savedboard.png");
  } else {
    outImagePB = load_pixbuf_from_file(themed_file_path(themeName,"bg.png"));
  }

  load_images(themeName);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_default_size(GTK_WINDOW(window), 1920, 1080);
  gtk_window_set_title(GTK_WINDOW(window), "Settlers");
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

  themeSelector = gtk_combo_box_new_text();
  add_theme_dirs(themeSelector);
  gtk_combo_box_set_active (GTK_COMBO_BOX (themeSelector), 0);
  gtk_widget_set_size_request(themeSelector, 70, 35);
  gtk_fixed_put(GTK_FIXED(frame), themeSelector, 20, 65);

  gtk_widget_show_all(window);
  
  progressBar = gtk_progress_bar_new();
  gtk_fixed_put(GTK_FIXED(frame), progressBar, 150, 65);
  gtk_widget_set_size_request(progressBar, 200, 35);
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progressBar), 0.1);

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

  g_signal_connect( G_OBJECT( themeSelector ), "changed",
      G_CALLBACK( cb_changed ), NULL );

  display_board();
  gtk_main();

  return 0;
}