.include "macros.inc"

.section .data

glabel _CatherineSegmentRomStart
.incbin "../models/binary/catherineMdl.bin"
.balign 16
glabel _CatherineSegmentRomEnd
