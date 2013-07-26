#!/usr/bin/env perl
use strict;
use warnings;
use GD;
use Tk;
use Tk::PNG;
use MIME::Base64 qw[ encode_base64 ];
# sudo yum -y install perl-Tk perl-GD 
# Grid generator: http://axiscity.hexamon.net/users/isomage/misc/svg-hex.cgi
my $theme = "wide";

# New method so we don't have to keep using  srcY, destW, destH, srcW, srcH
sub GD::Image::copyHex {
    my ($self) = shift;
    my $source = shift;
    my $dstX = shift;
    my $dstY = shift;
    my ($hexwidth,$hexheight) = $source->getBounds();
    #copyResampled(destination, source, dstX, dstY, srcX, srcY, destW, destH, srcW, srcH)
    # $self->copyResampled($source, $dstX, $dstY, 0, 0, $hexwidth, $hexheight, $hexwidth, $hexheight);
    my $angle = (int rand(6)) * 60;
   $self->copyRotated($source, $dstX + $hexwidth / 2, $dstY + $hexheight/2, 0, 0, $hexwidth, $hexheight, $angle);
}

sub fisher_yates_shuffle {
    my $deck = shift; # $deck is a reference to an array
    return unless @$deck; # must not be empty!
    my $i = @$deck;
    while (--$i) {
	my $j = int rand ($i+1);
	@$deck[$i,$j] = @$deck[$j,$i];
    }
}

GD::Image->trueColor(1);
# Load a 1920 x 1080 background/water image
my $bg = GD::Image->newFromPng("themes/$theme/bg.png");
$bg->alphaBlending(0); $bg->saveAlpha(1);

my ($bgW, $bgH) = $bg->getBounds();

# Load images of all the tiles
my $hexgrid = GD::Image->newFromPng("themes/$theme/hexgrid.png");
$hexgrid->alphaBlending(0); $hexgrid->saveAlpha(1);
my $wheat = GD::Image->newFromPng("themes/$theme/wheat.png");
$wheat->alphaBlending(0); $wheat->saveAlpha(1);
my $brick = GD::Image->newFromPng("themes/$theme/brick.png");
$brick->alphaBlending(0); $brick->saveAlpha(1);
my $ore = GD::Image->newFromPng("themes/$theme/rock.png");
$ore->alphaBlending(0); $ore->saveAlpha(1);
my $sheep = GD::Image->newFromPng("themes/$theme/sheep.png");
$sheep->alphaBlending(0); $sheep->saveAlpha(1);
my $tree = GD::Image->newFromPng("themes/$theme/tree.png");
$tree->alphaBlending(0); $tree->saveAlpha(1);
my $desert = GD::Image->newFromPng("themes/$theme/desert.png");
$desert->alphaBlending(0); $desert->saveAlpha(1);

my @numbersImage = ();
for (my $i = 2; $i<= 12; $i++ ){
    if ($i !=7 ) {
	my $temp = GD::Image->newFromPng("themes/$theme/" .$i . ".png");
	$numbersImage[$i] = $temp;
	$numbersImage[$i]->alphaBlending(0); $numbersImage[$i]->saveAlpha(1);
    }
}
$numbersImage[1]= GD::Image->newFromPng("themes/$theme/circle.png");
$numbersImage[7]= GD::Image->newFromPng("themes/$theme/circle.png");

my $ports4 = GD::Image->newFromPng("themes/$theme/ports4.png");
my $ports6 = GD::Image->newFromPng("themes/$theme/ports6.png");

# Calculate the display ofsets of the hexes loaded above
my ($hexwidth,$hexheight) = $brick->getBounds();
my $hexside = $hexheight  / 2;
my $offset = int(sqrt(3)* $hexside / 4) + 8;
print "$hexwidth,$hexheight,$hexside,$offset";

# Tile and numbersets are constant
my @deck4 = ($tree, $tree, $tree, $tree, 
	     $wheat, $wheat, $wheat, $wheat, 
	     $sheep, $sheep, $sheep, $sheep,
	     $ore, $ore, $ore,
	     $brick, $brick, $brick );
my @deck6 = ($tree, $tree, $tree, $tree, $tree, $tree, 
	    $wheat, $wheat, $wheat, $wheat, $wheat, $wheat, 
	    $sheep, $sheep, $sheep, $sheep,$sheep, $sheep,
	    $ore, $ore, $ore, $ore, $ore,
	    $brick, $brick, $brick, $brick, $brick),
my @numbersDeck4 = (2, 3, 3, 4, 4, 5, 5, 6, 6, 8, 8, 9, 9, 10, 10, 11, 11, 12, 7);
my @numbersDeck6 = (2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 8, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 12, 12, 7, 7);

#Start building the board image
my $outImage = new GD::Image($bgW, $bgH);
$outImage->alphaBlending(1);
$outImage->saveAlpha(1);
if ( -f "/tmp/settlersboard.png" ) {
    $outImage=GD::Image->newFromPng("/tmp/settlersboard.png");
} else {
    $outImage->copyResampled($bg, 0, 0, 0 ,0, $bgW, $bgH, $bgW, $bgH);
}
sub generate_board {
    my $numplayers = shift;
    my @deck = ();
    my @numbersDeck = ();
    my @board=();
    # Initialize Board array
    for (my $col=0; $col<=8; $col++) {
	for (my $row=0; $row<=8; $row++) {
	    $board[$col][$row] = 0;
	}
    }
    # Distribute numbers first until we find a valid config
    my $validboard = 0;
    while ($validboard == 0) {
	#print("Reset\n");
	if ($numplayers == 6 ) {
	    @deck = @deck6;
	    @numbersDeck = @numbersDeck6;
	} else {
	    @deck = @deck4;
	    @numbersDeck = @numbersDeck4;
	}
    fisher_yates_shuffle(\@deck);
    fisher_yates_shuffle(\@numbersDeck);
    $validboard = 1;
    # Only fill the proper spaces in the hex grid with numbers
    for (my $col=1; $col<=7; $col++) {
	for (my $row=1; $row<=6; $row++) {
	    if ($numplayers == 6 ) {
		if ( ($row == 1 || $row == 6 ) && ($col >= 2 && $col <= 5))  {
		    $board[$col][$row] = pop(@numbersDeck);
		}
		if ( ($row == 2 ) && ($col >= 2 && $col <= 6))  {
		    $board[$col][$row] = pop(@numbersDeck);
		}
		if ( ($row == 3 || $row == 4 ) && ($col >= 1 && $col <= 6))  {
		    $board[$col][$row] = pop(@numbersDeck);
		}
		if ( ($row == 5 ) && ($col >= 1 && $col <= 5))  {
		    $board[$col][$row] = pop(@numbersDeck);
		}
	    } else {
		if ( ($row == 2 || $row == 6 ) && ($col >= 2 && $col <= 4))  {
		    $board[$col][$row] = pop(@numbersDeck);
		}
		if ( ($row == 3 || $row == 5 ) && ($col >= 1 && $col <= 4))  {
		    $board[$col][$row] = pop(@numbersDeck);
		}
		if ( ($row == 4 || $row == 4 ) && ($col >= 1 && $col <= 5))  {
		    $board[$col][$row] = pop(@numbersDeck);
		}
	    }
	    # Check adjacency on 6's and 8's
	    my $currentnum = $board[$col][$row];
	    if ($currentnum == 6 || $currentnum == 8 ) {
		#print "number $col,$row  $board[$col][$row] ";
		my @adjnums = ();
		push (@adjnums, $board[$col+1][$row] );#       printf("C%d,%d ",$col+1,$row);
		push (@adjnums, $board[$col-1][$row] );#       printf("C%d,%d ",$col-1,$row);
		push (@adjnums, $board[$col][$row-1] );#       printf("C%d,%d ",$col,$row-1);
		push (@adjnums, $board[$col][$row+1]); #       printf("C%d,%d ",$col,$row+1);
		if ($row % 2 == 0) { 
		    push (@adjnums, $board[$col-1][$row - 1]);#printf("C%d,%d ",$col-1,$row-1);
		    push (@adjnums, $board[$col-1][$row + 1]);#printf("C%d,%d ",$col+1,$row-1);
		} else {
		    push (@adjnums, $board[$col+1][$row - 1]);#printf("C%d,%d ",$col-1,$row+1);
		    push (@adjnums, $board[$col+1][$row + 1]);#printf("C%d,%d ",$col+1,$row+1);
		}
		foreach my $number (@adjnums) {
		    if ($number == 6 || $number == 8) {
			$validboard = 0;
		    }
		}
		#print ": @adjnums\n";
	    }
	}
    }
}

#Numbers are now arranged in $board, generate an image and distribute tiles
#print "Generate board image\n";
    # Center the board withing the bg image
    my $xoff = 0;
    my $yoff = 0;
    my $portH = 0;
    my $portW = 0;

    if ($numplayers == 6) {
	$xoff = int(($bgW - $hexwidth * 7.5) / 2) ;
	$outImage->copyResampled($bg, 0, 0, 0 ,0, $bgW, $bgH, $bgW, $bgH);
	$yoff = int(.25 * $hexheight);
	($portW, $portH) = $ports6->getBounds();
	$outImage->copyResampled($ports6, -21, 0, 0, 0, $portW, $portH, $portW, $portH);
#	$yoff =  int(($bgH - $hexheight * 8.5)/2);
    } else {
	$xoff = int(($bgW - $hexwidth * 7.5) / 2) ;
	$yoff =  int(($bgH - $hexheight * 5.5)/2);
	$outImage->copyResampled($bg, 0, 0, 0 ,0, $bgW, $bgH, $bgW, $bgH);
	($portW, $portH) = $ports4->getBounds();
	$outImage->copyResampled($ports4, ($bgW - $portW) /2 - $hexwidth*1.5, ($bgH - $portH) /2, 0, 0, $portW, $portH, $portW, $portH);
    }

for (my $col=1; $col<=7; $col++) {
    for (my $row=1; $row<=6; $row++) {
	my $dstX = 0; my $dstY = 0;
	my $imagenumber = $board[$col][$row];
	if ($board[$col][$row] != 0 ) {
	    if ($row  % 2 == 0) {
		$dstX = $xoff + $hexwidth * ($col - 1);
		$dstY = $yoff + $hexside * 1.5 * ($row -1);
	    } else {
		$dstX = $xoff + $hexwidth / 2 + $hexwidth * ($col - 1);
		$dstY = $yoff + $hexside * 1.5 * ($row -1);
	    }
	    
	    my $tile = $desert;
	    if ($board[$col][$row] != 7 ) { 
		$tile = pop(@deck);
		$outImage->copyHex($tile, $dstX, $dstY);
		my $imagenumber = $board[$col][$row];
		$outImage->copyHex($numbersImage[$imagenumber], $dstX, $dstY);
	    } else {
		$outImage->copyHex($tile, $dstX, $dstY);
	    }		
	    #print "number $col,$row  $board[$col][$row]\n";
	};

    }
}

#$outImage->copyResampled($hexgrid, $xoff, $yoff, 0, 0, $hexgrid->getBounds(), $hexgrid->getBounds());
open OUTIMAGE, ">", "/tmp/settlersboard.png" or die "can't write $!";
binmode OUTIMAGE;
print OUTIMAGE $outImage->png;
close OUTIMAGE;
}

# Setup the gui
my $mw=tkinit;
my $canvas = $mw->Scrolled('Canvas', -width => 1920, -height => 1080)->pack(-expand=>1, -fill=>'both');
my $img = $mw->Photo( -data=>encode_base64($outImage->png), -format=>'png');
$canvas->createImage(0,0,  
                  -image => $img, 
          -anchor => 'nw',
          -tags => ['img'],
    );

my $but4 = $canvas -> Button(-text=>"4 Players", -command =>\&push_button4);
my $but6 = $canvas -> Button(-text=>"6 Players", -command =>\&push_button6);
my $quitbutton = $canvas -> Button(-text=>"Quit", -command => sub { exit });

$canvas->createWindow(50, 20, -window=>$but4);
$canvas->createWindow(150 ,20, -window=>$but6);
$canvas->createWindow(250 ,20, -window=>$quitbutton);

MainLoop;

sub push_button4 {
    #$ent -> insert(0,"Hello, ");
    generate_board(4);
    $img = $mw->Photo( -data=>encode_base64($outImage->png), -format=>'png');
    $canvas->createImage(0,0,
                  -image => $img,
          -anchor => 'nw',
          -tags => ['img'],
	);
}

sub push_button6 {
    #$ent -> insert(0,"Hello, ");
    generate_board(6);
    $img = $mw->Photo( -data=>encode_base64($outImage->png), -format=>'png');
    $canvas->createImage(0,0,
                  -image => $img,
          -anchor => 'nw',
          -tags => ['img'],
	);
}
