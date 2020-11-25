//This file contains the code that the master process will execute.

#include <iostream>
#include <mpi.h>
#include<math.h>
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

            startTime = MPI_Wtime();
            masterMPI_Horizontal(data, pixels);
            stopTime  = MPI_Wtime();
            break;
        case PART_MODE_STATIC_STRIPS_VERTICAL:

            startTime = MPI_Wtime();
            masterMPI_Vertical(data, pixels);
            stopTime = MPI_Wtime();
            break;
        case PART_MODE_STATIC_BLOCKS:

            startTime = MPI_Wtime();
            masterMPI_Block(data, pixels);
            stopTime = MPI_Wtime();
            break;
        case PART_MODE_STATIC_CYCLES_VERTICAL:

            startTime = MPI_Wtime();
            masterMPI_CyclicVertical(data, pixels);
            stopTime = MPI_Wtime();
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


void masterMPI_Horizontal(ConfigData *data, float *pixels)
{

    MPI_Status status;
    double computationStart = MPI_Wtime();

    int rows_per_process = data->height / data->mpi_procs;
    int avg_per_process = rows_per_process;

    int remaining_this_process = data->height % data->mpi_procs;
    int remaining = remaining_this_process;
    if (remaining > data->mpi_rank)
    {
        rows_per_process++;
    }

    for (int i = 0; i < rows_per_process; i++)
    {
        for (int j = 0; j < data->height; j++)
        {
            int row = i;
            int column = j;
            int baseIndex = 3 * (row * data->width + column);
            shadePixel(&(pixels[baseIndex]), row, column, data);
        }
    }

    int next = rows_per_process;

    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    double communicationStart = MPI_Wtime();

    for (int proc = 1; proc < data->mpi_procs; proc++)
    {

        int rows_in_single_process = avg_per_process;

        if (proc < remaining_this_process)
        {
            rows_in_single_process++;
        }

        int total_pixels = 3 * data->width * rows_per_process;

        double comm_recv_buf = 0.0;

        float *proc_pixels = new float[total_pixels];

        MPI_Recv(proc_pixels, total_pixels, MPI_FLOAT, proc, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&comm_recv_buf, 1, MPI_DOUBLE, proc, 0, MPI_COMM_WORLD, &status);

        if (comm_recv_buf > computationTime)
        {
            computationTime = comm_recv_buf;
        }

        for (int row = 0; row < rows_in_single_process; row++)
        {
            for (int column = 0; column < data->height; column++)
            {

                int baseIndex = 3 * (next * data->width + column);
                int procIndex = 3 * (row * data->width + column);

                pixels[baseIndex] = proc_pixels[procIndex];
                pixels[baseIndex + 1] = proc_pixels[procIndex + 1];
                pixels[baseIndex + 2] = proc_pixels[procIndex + 2];
            }
            next++;
        }
    }

    double communicationStop = MPI_Wtime();
    double communicationTime = communicationStop - communicationStart;

    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}

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

        int total_pixels = 3 * columns_in_single_process * data->height;

        double comm_recv_buf = 0.0;

        float *proc_pixels = new float[total_pixels];

        MPI_Recv(proc_pixels, total_pixels, MPI_FLOAT, proc, 0, MPI_COMM_WORLD, &status); 
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

                pixels[baseIndex] = proc_pixels[procIndex];
                pixels[baseIndex + 1] = proc_pixels[procIndex + 1];
                pixels[baseIndex + 2] = proc_pixels[procIndex + 2];
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

void masterMPI_CyclicVertical(ConfigData *data, float *pixels)
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


    int start_cycle = data->cycleSize * data->mpi_rank;
    int cycle_counter = data->cycleSize *data->mpi_procs; 

    for (int z = start_cycle; z < data->width; z+=cycle_counter)
    {
        for (int i = 0; i < data->height; i++)
        {
            for (int j = 0; j < data->width; j++)
            {
                int row = i;
                int column = j;
                if(column < z + data->cycleSize)
                {
                    int baseIndex = 3 * (row * data->width + column);
                    shadePixel(&(pixels[baseIndex]), row, column, data);
                }
                
            }
        }
    }


    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    double communicationStart = MPI_Wtime();

    for (int proc = 1; proc < data->mpi_procs; proc++)
    {
        int next = 0;
        int columns_in_single_process = 0;
        start_cycle = data->cycleSize * proc;
        for (int z = start_cycle; z < data->width; z += cycle_counter)
        { 
            for (int j = 0; j < data->width; j++)
            {
                int column = j;
                if (column < z + data->cycleSize)
                    columns_in_single_process++;
            }
        }

        int total_pixels = 3 * columns_in_single_process * data->height;

        double comm_recv_buf = 0.0;

        float *proc_pixels = new float[total_pixels];

        MPI_Recv(proc_pixels, total_pixels, MPI_FLOAT, proc, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&comm_recv_buf, 1, MPI_DOUBLE, proc, 0, MPI_COMM_WORLD, &status);

        

        for (int cycle = start_cycle; cycle < data->width; cycle+= cycle_counter)
        {
            for (int column = 0; column < data->width; column++)
            {
                for (int row = 0; row < (data->height); row++)
                {

                    if (column < cycle + data->cycleSize)
                    {
                        int baseIndex = 3 * (row * data->width + column);
                        int procIndex = 3 * (row + next*data->height);

                        pixels[baseIndex] = proc_pixels[procIndex];
                        pixels[baseIndex + 1] = proc_pixels[procIndex + 1];
                        pixels[baseIndex + 2] = proc_pixels[procIndex + 2];
                    }
                }
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

void masterMPI_Block(ConfigData *data, float *pixels)
{
    double computationStart = MPI_Wtime();

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
        if ((data->mpi_rank % each_proc_sqrt) == each_proc_sqrt-1)
        { 
            columns_per_process =columns_per_process +  remaining_columns;
        }
    }
    
    if (remaining_rows)
    {
        if (data->mpi_rank / (each_proc_sqrt) == (each_proc_sqrt)-1)
        { 
            rows_per_process =rows_per_process +  remaining_rows;
        }
    }

    int end_column = start_column + columns_per_process;
    int end_row = start_row + rows_per_process;

    for (int i = start_row; i < end_row; i++)
    {
        for (int j = start_column; j < end_column; j++)
        {         
            int row = i;
            int column = j;
            int baseIndex = 3 * (row * data->width + column);

            shadePixel(&(pixels[baseIndex]), row, column, data); 
        }
    }

    double computationStop = MPI_Wtime();
    double computationTime = computationStop - computationStart;

    
    double communicationTime = 0.0;

    for (int proc = 1; proc < data->mpi_procs; proc++)
    { 

       
        int proc_columns = data->width / (each_proc_sqrt);
        int proc_rows = data->height / (each_proc_sqrt);

        int proc_start_columns = (proc % (each_proc_sqrt)) * proc_columns;
        int proc_start_rows = (proc / (each_proc_sqrt)) * proc_rows;

        
        if (remaining_columns)
        {
            if (proc / (each_proc_sqrt) == (each_proc_sqrt) - 1)
            { 
                proc_columns += remaining_columns;
            }
        }
      
        if (remaining_rows)
        {
            if ((proc % (each_proc_sqrt)) == (each_proc_sqrt) - 1)
            { 
                proc_rows += remaining_rows;
            }
        }

        end_column = proc_start_columns + proc_columns;
        end_row = proc_start_rows + proc_rows;


        int total_pixels = 3 * proc_columns * proc_rows;

        float *proc_pixels = new float[total_pixels];

        MPI_Recv(proc_pixels, total_pixels, MPI_FLOAT, proc, 0, MPI_COMM_WORLD, &status);

        for (int i = proc_start_rows; i < end_row; i++)
        {
            for (int j = proc_start_columns; j < end_column; j++)
            { 
                int row = i;
                int column = j;


                int baseIndex = 3 * (row * data->width + column);
                int procIndex = 3 * ((row - proc_start_rows) * proc_columns + (column - proc_start_columns));

                pixels[baseIndex] = proc_pixels[procIndex];
                pixels[baseIndex + 1] = proc_pixels[procIndex + 1];
                pixels[baseIndex + 2] = proc_pixels[procIndex + 2];
            }
        }

        
    }

    //Print the times and the c-to-c ratio
    //This section of printing, IN THIS ORDER, needs to be included in all of the
    //functions that you write at the end of the function.
    std::cout << "Total Computation Time: " << computationTime << " seconds" << std::endl;
    std::cout << "Total Communication Time: " << communicationTime << " seconds" << std::endl;
    double c2cRatio = communicationTime / computationTime;
    std::cout << "C-to-C Ratio: " << c2cRatio << std::endl;
}
