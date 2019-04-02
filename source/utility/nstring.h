struct nstring {
    size_t length = 0;
    char*  data = nullptr;
};

inline u32 strlen_nullterm(char* str)
{
    u32 count = 0;
    if (str) {
        while (*str++) {
            ++count;
        }
    }
    return count;
}