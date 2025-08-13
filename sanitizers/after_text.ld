SECTIONS
{
  .data.index :
  {
    KEEP(*(.data.index .data.index.*))
  }
}
INSERT AFTER .text;
