
#pragma once

// NOTE: editing this header file does NOT trigger a rebuild
// on source files which depend on it.
// see "dependency_skil_list" in SConstruct

// TODO: refactor this into separate headers per-package

namespace ogm
{
namespace ErrorCode
{
    
// PARSER ERRORS
enum class P
{
    exopun=93,
    termtok=94,
    asstok=95,
    sttok=96,
    stpunc=97,
    stunkstartkw=98,
    enumtok=99,
    vartok=100,
    varkw=101,
    ifkw=102,
    paren=103,
    parend=104,
    fnargs=105,
    acc=107,
    tern=108,
    fnparen=109,
    litfnarg=111,
    litfnkw=112,
    litfnparen=113,
    litfnargs=114,
    hasstruct=115,
    litobtok=116,
    litfnbrace=117,
    litstruct=118,
    fndefname=119,
    ppmacrodef=120,
    litstructfield=121,
    unkproduction=130,
    pplinetokc=131,
    pplinetok=132,
    litarr=140,
    blockbrace=141,
    blockbraced=142,
    forkw=143,
    forparen=144,
    forparend=145,
    whilekw=146,
    repkw=147,
    dokw=148,
    dountkw=149,
    withkw=150,
    switchkw=151,
    switchbrace=152,
    switchbraced=153,
    switchcasekw=154,
    switchcasepunc=155,
};

// Compiler Errors
enum class C
{   
    macdepth=5,
    
    parsemacro=200,
    rowrite=201,
    lvalpoplim=202,
    stunexpect=203,
    fnmacid=204,
    scrargs=206,
    unkfnscr=207,
    switch1def=208,
    biwrite=209,
    binest=210,
    retval=211,
    accerr=212,
    accmapargs=213,
    accgridargs=214,
    acclistargs=215,
    hasstruct=216,
    haslitfn=217,
    enummissing=218,
    enumrep=219,
    accwrite=220,
    stlv=221,
    arridxc=223,
    arrcon=224,
    arrds=225,
    arrmixcow=226,
    arrgc=227,
    astlv=228,
    posmacro=229,
};

// PROJECT ERRORS
enum class F
{
    file=1000,
    filexml=1001,
    parsemacro=1002,
    unkres=1003,
    unres=1004,
    unkprojext=1005,
    unkresext=1006,
    noimg=1007,
    fonterr=1008,
    nbiact=1009,
    actargc=1010,
    unact=1011,
    evarg=1012,
    sprmissing=1013,
    mskmissing=1014,
    prtmissing=1015,
    objmissing=1016,
    bgmissing=1017,
    shrdiv=1024,
    resleafexists=1025,
    resleafnexists=1026,
    resdup=1027,
    colshape=1028,
    imgfile=1029,
    
    unev=1100,
    projparse=1101,
    pmodel=1102,
    fopen=1103,
    respath=1104,
    
    // ARF
    arfdim=1500,
    arftst=1501,
    arfoffset=1502,
    arfsep=1503,
    arfcc=1504,
    arfbgdet=1505,
    arfspd=1506,
    arftiled=1507,
    arfobjdet=1508,
    arfposdet=1509,
    arfscale=1510,
    arfunkview=1511,
    arfxbr=1512,
    arfsheetrange=1513,
    arfsheetdim=1514,
    arfsubimg=1515,
    arforg=1516,
    arfbbox=1517,
    arfcoltype=1518,
    arfproj=1520,
};

}
}