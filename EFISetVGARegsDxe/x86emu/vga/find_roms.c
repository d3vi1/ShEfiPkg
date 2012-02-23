#include "project.h"

static inline int
compare (uint8_t * a, uint8_t * b, int len)
{
  while (len--)
    {
      if (*a != *b)
        return 0;
      a++;
      b++;
    }
  return 1;
}

static inline int
compareb (uint8_t * a, uint8_t * b, int len)
{
  b += (len - 1);
  while (len--)
    {
      if (*a != *b)
        return 0;
      a++;
      b--;
    }
  return 1;
}

int
find_rom (struct rom *rom)
{
  uint8_t *start, *end;

  start = alloc_buf (128);
  free_buf (start);

  end = start;

  start -= (64 * 1024 * 1024);
  end += (64 * 1024 * 1024);

  LogPrint (L"Searching for ROM in 0x%08x-0x%08x\n", start, end);

  for (; start < end; ++start)
    {

      if (compareb (start, rom->sig, rom->sig_len))
        {
          uint32_t checksum = 0;
          uint8_t *base = start - rom->sig_offset;
          int i;

          LogPrint (L"signature match at 0x%08x\n", (uint32_t) start);

          checksum = crc32 (base, rom->rom_len);

          LogPrint (L"checksum is 0x%08x or should be 0x%08x\n",
                    checksum, rom->checksum);

          if (checksum == rom->checksum)
            {
              rom->base = base;
              return 1;
            }

        }


    }

  LogPrint (L"Failed to find  ROM\n");

  return 0;
}



int
find_roms (void)
{

  if (!find_rom (&video_rom))
    {
      LogPrint (L"VGA rom not found\n");
      return -1;
    }

  LogPrint (L"VGA rom found at 0x%08x\n", (uint32_t) video_rom.base);

  return 0;
}
