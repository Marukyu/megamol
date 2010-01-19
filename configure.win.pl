#
# configure.win.pl
# MegaMol Core
#
# Copyright (C) 2008-2010 by VISUS (Universitaet Stuttgart).
# Alle Rechte vorbehalten.
#
push @INC, "configperl";
require "configperl.inc";

my @pps = ();
my @fps = ();
my @cfps = ();
my @sps = ();

$a = PathParameter->new();
    $a->id("outbin");
    $a->description("Path to the global \"bin\" output directory");
    $a->placeholder("%outbin%");
    $a->autoDetect(0);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("outshader");
    $a->description("Path to the global \"shaders\" output directory");
    $a->placeholder("%outshader%");
    $a->autoDetect(0);
    push @pps, $a;
    
$a = PathParameter->new();
    $a->id("vislib");
    $a->description("Path to the vislib directory");
    $a->placeholder("%vislib%");
    $a->markerFile("vislib.sln\$");
    $a->relativeLocation("./");
    $a->autoDetect(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("visglut");
    $a->description("Path to the visglut directory");
    $a->placeholder("%visglutPath%");
    $a->markerFile("include/visglut.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("expat");
    $a->description("Path to the expat directory");
    $a->placeholder("%expatPath%");
    $a->markerFile("expat.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("libpng");
    $a->description("Path to the libpng directory");
    $a->placeholder("%pngPath%");
    $a->markerFile("png.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("zlib");
    $a->description("Path to the zlib directory");
    $a->placeholder("%zlibPath%");
    $a->markerFile("zlib.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    push @pps, $a;

$c = ConfigFilePair->new();
    $c->inFile("ExtLibs.vsprops.input");
    $c->outFile("ExtLibs.vsprops");
    push @cfps, $c;

Configure("MegaMol(TM) Core Configuration for Windows", ".megamol.core.win.cache", \@pps, \@fps, \@cfps, \@sps);

