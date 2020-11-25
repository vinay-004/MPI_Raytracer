//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>

#include "RayTrace.h"
#include "master.h"

void masterMain(ConfigData* data)
{
    //Depending on the partitioning scheme, different things will happen.
    //You should have a different function for each of the required 
    //schemes that returns some values that you need to handle.
    
    //Allocate space for the image on the master.
    float* pixels = new float[3 * data->width * data->height];

    //Execution time will be defined as how long it takes
    //for the given function to execute based on partitioning
    //type.
    double renderTime = 0.0, startTime, stopTime;

	//Add the required partitioning methods here in the case statement.
	//You do not need to handle all cases; the default will catch any
	//statements that are not specified. This switch/case statement is the
	//only place that you should be adding code in this function. Make sure
	//that you update the header files with the new functions that will be
	//called.
	//It is suggested that you use the same parameters to your functions as shown
	//in the sequential example below.
    switch (data->partitioningMode)
    {
        case PART_MODE_NONE:
            //Call the function that will handle this.
            startTime = MPI_Wtime();
            masterSequential(data, pixels);
            stopTime = MPI_Wtime();
            break;
        case PART_MODE_STATIC_STRIPS_HORIZONTAL:

            // startTime = MPI_Wtime();
            // masterMPI_Horizontal(data, pixels);
            // stopTime  = MPI_Wtime();
            break;
        case PART_MODE_STATIC_STRIPS_VERTICAL:

            startTime = MPI_Wtime();
            masterMPI_Vertical(data, pixels);
            stopTime = MPI_Wtime();
            break;
        case PART_MODE_STATIC_BLOCKS:

            // startTime = MPI_Wtime();
            // masterMPI_Block(data, pixels);
            // stopTime = MPI_Wtime();
            break;
        default:
            std::cout << "This mode (" << data->partitioningMode;
            std::cout << ") is not currently implemented." << std::endl;
            break;
    }

    renderTime = stopTime - startTime;
    std::cout << "Execution Time: " << renderTime << " seconds" << std::endl << std::endl;

    //After this gets done, save the image.
    std::cout << "Image will be save to: ";
    std::string file = generateFileName(data);
    std::cout << file << std::endl;
    savePixels(file, pixels, data);

    //Delete the pixel data.
    delete[] pixels; 
}

void masterSequential(ConfigData* data, float* pixels)
{
    //Start the computation time timer.
    double computationStart = MPI_Wtime();

    //Render the scene.
    for( int i = 0; i < data->height; ++i )
    {
        for( int j = 0; j < data->width; ++j )
        {
            int row = i;
            int column = j;

            //Calculate the index into the array.
            int baseIndex = 3 * ( row * data->width + column );

            //Call the function to shade the pixel.
            shadePixel(&(pixels[baseIndex]),row,column,data);
        }
    }

    //Stop the comp. timer
    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    //After receiving from all processes, the communication time will
    //be obtained.
    double communicationTime = 0.0;

    //Print the times and the c-to-c ratio
	//This section of printing, IN THIS ORDER, needs to be included in all of the
	//functions that you write at the end of the function.
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}

// void masterMPI_Horizontal(ConfigData *data, float *pixels)
// {

//     int avg_rows_per_process = data->height / data->mpi_procs;
//     int start_row = 0;
//     int end_row = 0;
//     MPI_Status status;

//     float* proc_pixel = new float[3 * data->width * data->height];
//     int total_pixels = 3 * data->width * data->height;
//     //send start row and column to all slave processes
//     for (int proc = 1; proc < data->mpi_procs; proc++) {   
        
//         end_row = start_row + avg_rows_per_process - 1;
//         // [start_column, end_column]
//         // share proc_pixel array with processes 
//         MPI_Send(&start_row, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);
//         MPI_Send(&end_row, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);
//         MPI_Send(&proc_pixel, total_pixels, MPI_FLOAT, proc, 0, MPI_COMM_WORLD);

//         std::cout << "start_column : " << start_row << std::endl;
//         std::cout << "end_column : " << end_row << std::endl;
//         std::cout << "pixel : " << proc_pixel[0] << std::endl;

//         start_row += avg_rows_per_process;
//     }

//     std::cout<<"SENDING DONE"<<std::endl;
//     // render the remaining pixel data in master
//     // all the rows from 0 to height
//     for (int i = start_row; i < data->height; ++i) {
//         // only the remaining columns in the master
//         for (int j = 0; j < data->width; ++j) {
//             int row = i;
//             int column = j;

//             //Calculate the index into the array.
//             int baseIndex = 3 * (row * data->width + column);

//             //Call the function to shade the pixel.
//             shadePixel(&(pixels[baseIndex]), row, column, data);
//         }
//     }

//     std::cout<<" DONE WITH COMPUTATION"<<std::endl;
    
//     start_row = 0;
//     end_row = 0;

//     for (int proc = 1; proc < data->mpi_procs; proc++) {

//         MPI_Recv(&proc_pixel, total_pixels, MPI_FLOAT, proc, 1, MPI_COMM_WORLD, &status);

//         end_row = start_row + avg_rows_per_process -1;
//         for (int i = start_row; i < end_row; ++i) {
//             // only the remaining columns in the master
//             for (int j = 0; j <= data->width; ++j) {
//                 int row = i;
//                 int column = j;

//                 //Calculate the index into the array.
//                 int baseIndex = 3 * (row * data->width + column);

//                 //Call the function to shade the pixel.
//                 pixels[baseIndex] = proc_pixel[baseIndex];
//                 pixels[baseIndex + 1] = proc_pixel[baseIndex+1];
//                 pixels[baseIndex + 2] = proc_pixel[baseIndex + 2];
//             }
//         }
//         start_row += avg_rows_per_process;
//     }
// }

void masterMPI_Vertical(ConfigData *data, float *pixels)
{

    MPI_Status status;
    double computationStart = MPI_Wtime();


    int columns_per_process = (data->width) / (data->mpi_procs);
    int avg_per_process = columns_per_process;

    int remaining_this_process = data->width % data->mpi_procs;
    int remaining = remaining_this_process;
    if (remaining > data->mpi_rank)
    {             
        columns_per_process++;     
    }

    for (int i = 0; i < data->width; i++)
    { 
        for (int j = 0; j < columns_per_process; j++)
        { 
            int row = i;
            int column = j;
            int baseIndex = 3 * (row * data->width + column);
            shadePixel(&(pixels[baseIndex]), row, column, data); 
        }
    }

    int next = columns_per_process;

    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    
    double communicationStart = MPI_Wtime();

    for (int proc = 1; proc < data->mpi_procs; proc++)
    {

        int columns_in_single_process = avg_per_process;

        if (proc < remaining_this_process)
        {                
            columns_in_single_process++; 
        }

        int recv_buf_size = 3 * columns_in_single_process * data->height;

        double comm_recv_buf = 0.0;

        float *recv_buf = new float[recv_buf_size];

        MPI_Recv(recv_buf, recv_buf_size, MPI_FLOAT, proc, 0, MPI_COMM_WORLD, &status); 
        MPI_Recv(&comm_recv_buf, 1, MPI_DOUBLE, proc, 0, MPI_COMM_WORLD, &status);      

        if (comm_recv_buf > computationTime)
        {                                    
            computationTime = comm_recv_buf; 
        }

        for (int row = 0; row < (data->height); row++)
        {
            next = 0;
            for (int column = 0; column < columns_in_single_process; column++)
            { 
             

                int baseIndex = 3 * (row * data->width + next);
                int procIndex = 3 * (row * columns_in_single_process + column);

                pixels[baseIndex] = recv_buf[procIndex];
                pixels[baseIndex + 1] = recv_buf[procIndex + 1];
                pixels[baseIndex + 2] = recv_buf[procIndex + 2];
                next++;
            }
            
        }
    }

  
    double communicationStop = MPI_Wtime();
    double communicationTime = communicationStop - communicationStart;

    
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}

// //void masterMPI_Block(ConfigData *data, float *pixels)
// {

//     int avg_rows_per_process = data->height / data->mpi_procs;
//     int avg_columns_per_process = data->width / data->mpi_procs;
//     int start_column = 0;
//     int end_column = 0;
//     int start_row = 0;
//     int end_row = 0;
//     MPI_Status status;

//     float *proc_pixel = new float[3 * data->width * data->height];
//     int total_pixels = 3 * data->width * data->height;
//     //send start row and column to all slave processes
//     for (int proc = 1; proc < data->mpi_procs; proc++)
//     {

//         end_row = start_row + avg_rows_per_process -1;
//         end_column = start_column + avg_columns_per_process - 1;

//         if(end_row == data->height-1)
//         {
//             start_row = 0; 
//         }
//         if(end_column == data->width - 1)
//         {
//             end_column = 0;
//         }        
//         // [start_column, end_column]
//         // share proc_pixel array with processes
//         MPI_Send(&start_row, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);
//         MPI_Send(&end_row, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);

//         MPI_Send(&start_column, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);
//         MPI_Send(&end_column, 1, MPI_INT, proc, 0, MPI_COMM_WORLD);
//         MPI_Send(&proc_pixel, total_pixels, MPI_FLOAT, proc, 0, MPI_COMM_WORLD);

//         std::cout << "start_column : " << start_column << std::endl;
//         std::cout << "end_column : " << end_column << std::endl;
//         std::cout << "pixel : " << proc_pixel[0] << std::endl;

//         start_column += avg_columns_per_process;
//     }

//     std::cout << "SENDING DONE" << std::endl;
//     // render the remaining pixel data in master
//     // all the rows from 0 to height
//     // for (int i = 0; i < data->height; ++i)
//     // {
//     //     // only the remaining columns in the master
//     //     for (int j = start_column; j < data->width; ++j)
//     //     {
//     //         int row = i;
//     //         int column = j;

//     //         //Calculate the index into the array.
//     //         int baseIndex = 3 * (row * data->width + column);

//     //         //Call the function to shade the pixel.
//     //         shadePixel(&(pixels[baseIndex]), row, j, data);
//     //     }
//     // }

//     std::cout << " DONE WITH COMPUTATION" << std::endl;

//     start_column = 0;
//     end_column = 0;

//     start_row = 0;
//     end_row = 0;

//     for (int proc = 1; proc < data->mpi_procs; proc++)
//     {

//         MPI_Recv(&proc_pixel, total_pixels, MPI_FLOAT, proc, 1, MPI_COMM_WORLD, &status);

//         end_row = start_row + avg_rows_per_process - 1;
//         end_column = start_column + avg_columns_per_process - 1;
//         if(end_row == data->height -1)
//         {
//             start_row = 0;
//         }
//         if(end_column == data->width -1)
//         {
//             start_column = 0;
//         }
        
//         for (int i = start_row; i < end_row; ++i)
//         {
//             // only the remaining columns in the master
//             for (int j = start_column; j <= end_column; ++j)
//             {
//                 int row = i;
//                 int column = j;

//                 //Calculate the index into the array.
//                 int baseIndex = 3 * (row * data->width + column);

//                 //Call the function to shade the pixel.
//                 pixels[baseIndex] = proc_pixel[baseIndex];
//                 pixels[baseIndex + 1] = proc_pixel[baseIndex + 1];
//                 pixels[baseIndex + 2] = proc_pixel[baseIndex + 2];
//             }
//         }
//         start_column += avg_columns_per_process;
//     }
// }
