//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include "RayTrace.h"
#include "slave.h"

void slaveMain(ConfigData* data)
{
    //Depending on the partitioning scheme, different things will happen.
    //You should have a different function for each of the required 
    //schemes that returns some values that you need to handle.

    
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //The slave will do nothing since this means sequential operation.
            break;
        case PART_MODE_STATIC_STRIPS_HORIZONTAL:
            //slaveMPIHorizontal(data);
            break;
        case PART_MODE_STATIC_STRIPS_VERTICAL:
            slaveMPIVertical(data);
            break;
        case PART_MODE_STATIC_BLOCKS:
            //slaveMPIBlock(data);
            break;
        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }
}

// void slaveMPIHorizontal(ConfigData *data, float *pixels)
// {

//     MPI_Status status;
//     int start_row;
//     int end_row;

//     int total_pixels = 3 * data->width * data->height;

//     MPI_Recv(&start_row, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
//     MPI_Recv(&end_row, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
//     MPI_Recv(&pixels, total_pixels, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);

//     std::cout << "start_row : " << start_row << std::endl;
//     std::cout << "end_row : " << end_row << std::endl;
//     std::cout << "pixel : " << pixels[0] << std::endl;

//     //Render the scene.
//     for (int i = start_row; i < end_row; ++i)
//     {
//         for (int j = 0; j <= data->height; ++j)
//         {
//             int row = i;
//             int column = j;

//             //Calculate the index into the array.
//             int baseIndex = 3 * (row * data->width + column);

//             //Call the function to shade the pixel.
//             shadePixel(&(pixels[baseIndex]), row, column, data);
//         }
//     }

//     MPI_Send(&pixels, total_pixels, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
// }

void slaveMPIVertical(ConfigData *data) {

    double computationStart = MPI_Wtime();

    int my_cols = (data->width) / (data->mpi_procs);

    if (data->mpi_rank < (data->width % data->mpi_procs))
    {              
        my_cols++; 
    }

    int start_col = data->mpi_rank * my_cols - 1; 
    if (data->width % data->mpi_procs)
    { 
        if (data->mpi_rank - (data->width % data->mpi_procs) < 0)
        { 
            start_col++;
        }
        else
        { 
            start_col += (data->width % data->mpi_procs) + 1;
        }
    }

    float *my_pixels = new float[3 * data->height * my_cols];

    
    int my_column = 0;
    for (int column = start_col; column < start_col + my_cols; column++)
    { 

        for (int row = 0; row < data->height; row++)
        { 

           
            int baseIndex = 3 * (row * my_cols + my_column);
            shadePixel(&(my_pixels[baseIndex]), row, column, data); 
        }
        my_column++;
    }
    
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    MPI_Send(my_pixels, 3 * data->width * my_cols, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(&computationTime, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

    delete[] my_pixels;
}


// void slaveMPIBlock(ConfigData *data, float *pixels)
// {

//     MPI_Status status;
//     int start_column;
//     int end_column;

//     int start_row;
//     int end_row;

//     int total_pixels = 3 * data->width * data->height;

//     MPI_Recv(&start_row, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
//     MPI_Recv(&end_row, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

//     MPI_Recv(&start_column, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
//     MPI_Recv(&end_column, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
//     MPI_Recv(&pixels, total_pixels, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);

//     std::cout << "start_column : " << start_column << std::endl;
//     std::cout << "end_column : " << end_column << std::endl;
//     std::cout << "pixel : " << pixels[0] << std::endl;

//     //Render the scene.
//     for (int i = start_row; i < end_row; ++i)
//     {
//         for (int j = start_column; j <= end_column; ++j)
//         {
//             int row = i;
//             int column = j;

//             //Calculate the index into the array.
//             int baseIndex = 3 * (row * data->width + column);

//             //Call the function to shade the pixel.
//             shadePixel(&(pixels[baseIndex]), row, column, data);
//         }
//     }

//     MPI_Send(&pixels, total_pixels, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
// }
