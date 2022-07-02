#include <iostream>
#include <vector>
#include "mpi.h"
#include <C:\MPI\HelloMPI\HelloMPI\gif.h>
using namespace std;

bool mandelbrot(float imC, float reC, int& i)
{
    i = 0;
    const int maxIter = 100;
    float d = 0.0f;
    float imZ = 0.0f;
    float reZ = 0.0f;
    for (; i < maxIter && d <= 2.0f; ++i)
    {
        float im = imZ * imZ - reZ * reZ + imC;
        float re = 2.0f * imZ * reZ + reC;
        imZ = im;
        reZ = re;
        d = sqrt(imZ * imZ + reZ * reZ);
    }
    return d < 2;
}

void CreateFrames(vector<uint8_t>& images, int fst, int snd, int height, int width)
{
    int index;
    float red, grn, blu;
    for (int frame = fst; frame < snd; ++frame)
    {
        for (int i = 0; i < height; ++i)
        {
            for (int j = 0; j < width; ++j)
            {
                if (mandelbrot((-2 + j * (3.0 / width)) * (1.0 - 0.005 * frame) - 0.5, (1 - i * (2.0 / height)) * (1.0 - 0.005 * frame) + 0.5, index))
                {
                    red = grn = blu = 220;
                }
                else
                {
                    red = (index * 251) % 255;
                    grn = (index * 251) % 255;
                    blu = (index * 254) % 255;
                }
                images.push_back((uint8_t)roundf(255.0f * red)); 
                images.push_back((uint8_t)roundf(255.0f * grn)); 
                images.push_back((uint8_t)roundf(255.0f * blu));
                images.push_back(255);
            }
        }
    }
}

int main(int argc, char** argv)
{
    const int height = 512;
    const int width = 768;
    const int frame_num = 16;
    //
    MPI_Init(&argc, &argv);
    int numstacks, rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numstacks);
    //
    double start_time;
    if (rank == 0)
    {
        printf("frame count: %d\nnumstacks: %d\n\nloading...\n\n", frame_num, numstacks);
        start_time = MPI_Wtime();
    }
    //
    int fst = frame_num / numstacks * rank;
    int snd = frame_num / numstacks * (rank + 1);
    //
    vector<uint8_t> temp;
    //
    CreateFrames(temp, fst, snd, height, width);
    //
    if (rank != 0)
    {
        MPI_Send(temp.data(), temp.size(), MPI_UINT8_T, 0, 0, MPI_COMM_WORLD);
    }
    else
    {
        GifWriter writer = {};
        GifBegin(&writer, "mygif.gif", width, height, 2, 8, true);
        for (int i = 0; i < frame_num / numstacks; i++)
            GifWriteFrame(&writer, temp.data() + height * width * 4 * i, width, height, 2, 8, true);

        uint8_t* buffer = new uint8_t[temp.size()];
        for (int i = 1; i < numstacks; i++)
        {
            MPI_Recv(buffer, temp.size(), MPI_UINT8_T, i, 0, MPI_COMM_WORLD, MPI_STATUSES_IGNORE);
            for (int i = 0; i < frame_num / numstacks; i++)
                GifWriteFrame(&writer, buffer + height * width * 4 * i, width, height, 2, 8, true);
        }
        double res_time = MPI_Wtime() - start_time;
        printf("ready! time: %f sec", res_time);
        GifEnd(&writer);
    }
    MPI_Finalize();
    return 0;
}
