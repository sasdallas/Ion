EXTERN(__ImageBase);

SECTIONS
{
    __ImageEnd = ALIGN(., __section_alignment__);
}
INSERT AFTER .reloc;
