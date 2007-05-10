void flushcache(unsigned int beginning_addr, unsigned int end_addr, unsigned int flags);
void block_or(void *src, size_t n, int pat);
void block_andor(void *src, size_t n, int andpat, int orpat);
void spend_cycles(int c); // utility

