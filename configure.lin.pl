#
# configure.lin.pl
# MegaMol Plugin
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
    $a->id("mmcore");
    $a->description("Path to the MegaMol core directory");
    $a->placeholder("%mmcorePath%");
    $a->markerFile("api/MegaMolCore.h\$");
    $a->relativeLocation("../");
    $a->autoDetect(1);
    push @pps, $a;
$a = PathParameter->new();
    $a->id("vislib");
    $a->description("Path to the vislib directory");
    $a->placeholder("%vislib%");
    $a->markerFile("vislib.sln\$");
    $a->relativeLocation("./");
    $a->autoDetect(1);
    push @pps, $a;

$c = ConfigFilePair->new();
    $c->inFile("ExtLibs.mk.input");
    $c->outFile("ExtLibs.mk");
    push @cfps, $c;

Configure("MegaMol(TM) Plugin Configuration for Linux", ".megamol.plg.lin.cache", \@pps, \@fps, \@cfps, \@sps);

