
#include <iostream>
#include "sockets/Udp.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "src/Driver.h"
#include "src/Serializer.h"
#include "src/StringParser.h"
#include <cstdlib>

void closeClient(Taxi *taxi, Driver *driver, Vehicle *vehicle, Socket *socket);

int main(int argc, char *argv[]) {

    //The socket connecting between the client and the server.
    Socket       *socket    = new Udp(0, atoi(argv[1]), argv[2]);
    //Serializer used for serializing and deserializing objects.
    Serializer   serializer;
    //Will handle parsing the user input.
    StringParser stringParser;
    Driver       *driver    = 0;
    Vehicle      *vehicle   = 0;
    Taxi         *taxi      = 0;
    Trip         *trip;
    bool         exitCalled = false;

    socket->initialize();

    //Creates a driver from a given user input.
    driver = stringParser.parseDriverInput();

    //Serializes the driver into a string.
    std::string serialDriver = serializer.serialize(driver);

    std::cout << serialDriver << std::endl; //TODO delete

    //Sends the string to the server
    socket->sendData(serialDriver);

    char buffer[1024];

    //Receives data from the server.
    socket->reciveData(buffer, sizeof(buffer));
    std::cout << buffer << endl;

    //Deserializes the data received from the server into a vehicle object.
    serializer.deserialize(buffer, sizeof(buffer), vehicle);

    std::cout << vehicle->getCoefficient() << std::endl; //TODO delete

    taxi = new Taxi(driver, vehicle, Point(0, 0));
    serialDriver = "";

    //Take input from the server while it's not an exit command.
    while (!exitCalled) {

        char communicationBuffer[1024];

        socket->reciveData(communicationBuffer, sizeof(communicationBuffer));
        std::cout << communicationBuffer << std::endl;

        //If received "exit", close the client.
        if (strcmp(communicationBuffer, "exit") == 0) {

            exitCalled = true;
        }

        //If received "go", move the taxi towards its destination.
        else if (strcmp(communicationBuffer, "go") == 0) {

            taxi->move();
            std::string serial = serializer.serialize(&taxi->getCurrentPosition());
            socket->sendData(serial);

            /*
            //If the taxi finished the trip, notify the server.
            if (!taxi->hasTrip()) {

                serialDriver = serializer.serialize(taxi->getDriver());
                socket->sendData(serialDriver);
            }
            else{

                //Notify the server that the command was received.
                std::string serial = serializer.serialize(&taxi->getCurrentPosition());
                socket->sendData(serial);
            }
             */
        }

        else {

            trip = 0;
            //Set the received trip from the server in the taxi.
            serializer.deserialize(communicationBuffer, sizeof(communicationBuffer), trip);
            taxi->setTrip(trip);
        }

    }

    //Close the client process.
    closeClient(taxi, driver, vehicle, socket);
}

void closeClient(Taxi *taxi, Driver *driver, Vehicle *vehicle, Socket *socket){
    delete taxi;
    delete driver;
    delete vehicle;
    delete socket;
    exit(0);
}