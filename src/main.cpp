/*
 * main.cpp
 *
 *  Created on: 29.12.2020
 *      Author: DI Andreas Auer
 */

#include "tunnel.h"

#include <iostream>

int main(int argc, char *argv[])
{
    try
        {
        Tunnel tunnel;
        tunnel.run(argc, argv);
    }
    catch(std::exception &e)
    {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
