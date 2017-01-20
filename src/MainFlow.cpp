
#include "MainFlow.h"
#include "../sockets/Tcp.h"
#include "TripThread.h"
//#include <boost/log/trivial.hpp>
#include <cstdlib>
#include <pthread.h>

MainFlow::MainFlow(Socket *socket, int operationNumber) {

    this->socket = socket;
    //BOOST_LOG_TRIVIAL(info) << "Opening main socket.";
    this->socket->initialize();
    this->operationNumber = &operationNumber;
    this->threadIdQueue = new queue<int>();

    pthread_mutex_init(&this->receiveDriverMutex, NULL);
    pthread_mutex_init(&this->sendCommandMutex, NULL);
    pthread_mutex_init(&this->bfsMutex, NULL);

}

MainFlow::~MainFlow() {

    pthread_mutex_destroy(&this->receiveDriverMutex);
    pthread_mutex_destroy(&this->sendCommandMutex);
    pthread_mutex_destroy(&this->bfsMutex);

    delete this->threadIdQueue;

    //BOOST_LOG_TRIVIAL(info) << "Deleting all the open sockets.";
    delete this->socket;

}

int *MainFlow::getOperationNumber() {
    return this->operationNumber;
}

void MainFlow::createMap(Grid *grid) {
    this->map = grid;
}

void MainFlow::createTaxiCenter(Point *location) {

    this->taxiCenter = new TaxiCenter(location);
}

void MainFlow::createDriver(Driver *driver) {

    taxiCenter->addDriver(driver);
}

void MainFlow::createVehicle(Vehicle *vehicle) {

    taxiCenter->addVehicle(vehicle);
}

void MainFlow::createTrip(Trip *trip) {

    int       retVal = 0;
    pthread_t pthread;

    TripThread *tripThread = new TripThread(this, trip);

    retVal = pthread_create(&pthread, NULL, &TripThread::callCalculatePath,
                            tripThread);

    if (retVal != 0) {

        //BOOST_LOG_TRIVIAL(error) << "Failed to create trip thread.";
    }

    tripThread->setThread(pthread);

    //BOOST_LOG_TRIVIAL(info) << "Created trip thread.";

    this->getTaxiCenter()->getTripThreads().push_back(tripThread);
    this->taxiCenter->addTrip(trip);

}

TaxiCenter *MainFlow::getTaxiCenter() const {
    return taxiCenter;
}

void MainFlow::sendToSocketVehicle(unsigned int vehicleId,
                                   int descriptor) {

    Vehicle     *vehicle = this->getDriverVehicle(vehicleId);
    std::string serialVehicle;
    serialVehicle = this->serializer.serialize(vehicle);

    socket->sendData(serialVehicle, descriptor);
}

Vehicle *MainFlow::getDriverVehicle(unsigned int vehicleId) {

    std::vector<Vehicle *> vehicles = this->getTaxiCenter()->
            getVehicles();

    for (int i = 0; i < vehicles.size(); ++i) {

        if (vehicles[i]->getVehicleId() == vehicleId) {

            return vehicles[i];
        }
    }
}

std::queue<int> *MainFlow::getThreadIdQueue() {
    return this->threadIdQueue;
}

void MainFlow::addClientId(int threadId) {
    this->threadIdQueue->push(threadId);
}


void MainFlow::selectDrivers(int numOfDrivers) {

    // Receive driver objects from client
    for (int i = 0; i < numOfDrivers; i++) {

        pthread_t currThread;

        ClientThread *clientThread = new ClientThread(this, i);

        this->addClientThread(clientThread);

        // Add client ID to queue
        this->addClientId(i);

        // Init thread for driver
        pthread_create(&currThread, NULL,
                       ClientThread::sendToListenToSocketForDriver,
                       (void *) (clientThread));

        //BOOST_LOG_TRIVIAL(info) << "New thread created with thread id: "
         //                       << clientThread->getThreadId();
        clientThread->setThread(currThread);

    }
}

void MainFlow::performTask9(Driver *driver, int descriptor) {

    // Check that all the trips that need to start are attached
    // to a driver
    //BOOST_LOG_TRIVIAL(debug) << "Assigning driver: " << driver->getDriverId() << " a trip";
    this->getTaxiCenter()->assignTrip(driver, *this->getSocket(), this->getSerializer(),
                                      descriptor);

                                      // Move all the taxis one step
    this->getTaxiCenter()->moveOneStep(driver,
            *(this->getSocket()), this->getSerializer(), descriptor);

}

unsigned int MainFlow::getVacantPort() {
    return this->vacantPort;
}

void MainFlow::increaseVacantPort() {
    ++(this->vacantPort);
}

Socket *MainFlow::getSocket() {
    return this->socket;
}

Serializer MainFlow::getSerializer() {
    return this->serializer;
}

void MainFlow::addClientThread(ClientThread *clientThread) {
    this->clientThreadVector.push_back(clientThread);
}

std::vector<ClientThread *> MainFlow::getClientThreadVector() {
    return this->clientThreadVector;
}

void MainFlow::sendClientNewPort(unsigned int newPort) {

    // Case newPort to string and send the new socket to client
    // this->getSocket()->sendData(boost::lexical_cast<string>(newPort));
}

Grid *MainFlow::getMap() const {
    return map;
}

void MainFlow::exitSystem() {

    delete this->taxiCenter;
    delete this->map;

    //exit the system
    exit(0);
}

void MainFlow::cleanGrid() {

    Vertex *v;

    for (int i = 0; i < this->map->get_edges().size(); ++i) {

        v = this->map->get_vertex(map->get_edges()[i].getM_point());

        if (!this->getMap()->isObstacle(v->getM_point())) {

            v->set_unvisited();
        }

        v->setFather(0);
    }
}

pthread_mutex_t &MainFlow::getMutexReceiveDriver() {
    return receiveDriverMutex;
}

pthread_mutex_t &MainFlow::getSendCommandMutex()  {
    return sendCommandMutex;
}

void MainFlow::clockSleep() {

    int i = 0;
    while(i < this->getClientThreadVector().size()){

        if(this->getClientThreadVector()[i]->getThreadCommand() == 0){
            i++;
        }
    }

}
