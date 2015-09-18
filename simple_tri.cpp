#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdio>

float result[1024];

int main(int argc, char** argv)
{
    FILE* fh = std::fopen("simple_tri.txt", "wb");
    for (int iter = 0; iter < 50000; iter++)
    {
        for (int i = 0; i < 1024; i++)
        {
            float tmp = i;
            result[i] = std::tan(tmp) + std::tan(2*tmp) + std::sin(tmp) + std::cos(tmp);
        }

        std::fwrite(result, sizeof(float), 1024, fh);

        // reset result
        for (int i = 0; i < 1024; i++)
        {
            result[i] = 0.0f;
        }

        // write again
        std::fwrite(result, sizeof(float), 1024, fh);
    }

    fclose(fh);
}
