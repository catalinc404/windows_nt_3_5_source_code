# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the BLT test app XVALID.EXE


# These two macros give the class source/executable name and
# dialog file name.

BLTTEST=xfont
BLTTESTRC_DEPEND=xfont.rc testfont.h testfont.ico


!include $(UI)\common\src\blt\test\rules.mk
