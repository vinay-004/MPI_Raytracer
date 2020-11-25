//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include "RayTrace.h"
#include "slave.h"
#include<math.h>

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
            slaveMPIHorizontal(data);
            break;
        case PART_MODE_STATIC_STRIPS_VERTICAL:
            slaveMPIVertical(data);
            break;
        case PART_MODE_STATIC_BLOCKS:
            slaveMPIBlock(data);
            break;
        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }
}


void slaveMPIHorizontal(ConfigData *data)
{

    double computationStart = MPI_Wtime();

    int rows_per_process = (data->height) / (data->mpi_procs);
    int remaining = data->height % data->mpi_procs;

    if (remaining > data->mpi_rank)
    {
        rows_per_process++;
    }

    int start_row = data->mpi_rank * rows_per_process - 1;

    if (remaining)
    {
        if (data->mpi_rank - (remaining) < 0)
        {
            start_row++;
        }
        else
        {
            start_row = start_row + remaining + 1;
        }
    }

    int total_pixels = 3 * rows_per_process * data->width;
    float *pixels = new float[total_pixels];
    int end_row = start_row + rows_per_process;
    int next = 0;
    for (int i = start_row; i < end_row; i++)
    {
        for (int j = 0; j < data->width; j++)
        {
            int row = i;
            int column = j;
            int baseIndex = 3 * (next * data->width + column);

            shadePixel(&(pixels[baseIndex]), row, column, data);
        }
        next++;
    }

    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    MPI_Send(pixels, total_pixels, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(&computationTime, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
}

void slaveMPIVertical(ConfigData *data) {

    double computationStart = MPI_Wtime();

    int columns_per_process = (data->width) / (data->mpi_procs);
    int remaining = data->width % data->mpi_procs;

    if (remaining > data->mpi_rank)
    {              
        columns_per_process++; 
    }

    int start_column = data->mpi_rank * columns_per_process - 1; 

    if (remaining)
    { 
        if (data->mpi_rank - (remaining) < 0)
        { 
            start_column++;
        }
        else
        { 
            start_column =start_column  + remaining + 1;
        }
    }

    int total_pixels = 3 *data->height *columns_per_process;
    float *pixels = new float[total_pixels];
    int end_column = start_column + columns_per_process;
    int next = 0;
    for (int i = 0; i < data->height; i++)
    {
        next = 0;
        for (int j = start_column; j < end_column; j++)
        { 
            int row = i;
            int column = j;
            int baseIndex = 3 * (row * columns_per_process + next);

            shadePixel(&(pixels[baseIndex]), row, column, data); 
            next++;
        }
        
    }
    
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    MPI_Send(pixels, total_pixels, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);
    MPI_Send(&computationTime, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

    
}


void slaveMPIBlock(ConfigData *data)
{
    double computationTime = MPI_Wtime();

    MPI_Status status;
    int each_proc_sqrt = (int)sqrt(data->mpi_procs);
    int columns_per_process = data->width / each_proc_sqrt;
    int rows_per_process = data->height / each_proc_sqrt;

    int remaining_columns = data->width % each_proc_sqrt;
    int remaining_rows = data->height % each_proc_sqrt;

    int start_column = (data->mpi_rank / (each_proc_sqrt)) * columns_per_process;
    int start_row = (data->mpi_rank % (each_proc_sqrt)) * rows_per_process;

    if (remaining_columns)
    {
        if ((data->mpi_rank % (each_proc_sqrt)) == (each_proc_sqrt)-1)
        {
            columns_per_process += remaining_columns;
        }
    }

    if (remaining_rows)
    {
        if (data->mpi_rank / (each_proc_sqrt) == (each_proc_sqrt)-1)
        {
            rows_per_process += remaining_rows;
        }
    }

    int end_column = start_column + columns_per_process;
    int end_row = start_row + rows_per_process;

    int totat_pixels = 3 * columns_per_process * rows_per_process;
    float *pixels = new float[totat_pixels];
    for (int i = start_row; i < end_row; i++)
    {
        for (int j = start_column; j < end_column; j++)
        {
            int row = i;
            int column = j;
            
            int base_row = row - start_row;
            int base_column = column - start_column;

            int baseIndex = 3 * (base_row * columns_per_process + base_column);

            shadePixel(&(pixels[baseIndex]), row, column, data);
        }
    }

    MPI_Send(&pixels, totat_pixels, MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
}
