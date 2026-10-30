#ifndef __I_VESC_COMMUNICATION_H__
#define __I_VESC_COMMUNICATION_H__

#include <Arduino.h>
#include "VescData.h"
#include "VescCommandParser.h"

static const char* LOG_TAG_VESC_COMMUNICATION = "VescComm";

/**
 * Transport independent interface to talk to the VESC.
 *
 * It abstracts over the two supported physical transports (CANBUS and UART)
 * so that components like the BleServer or the main loop never need to know
 * which one is active. Implementations are created by VescCommunicationFactory.
 */
class IVescCommunication {
  public:
    virtual ~IVescCommunication() = default;

    virtual void init() = 0;
    virtual void loop() = 0;

    /** BLE -> VESC, forwards client data to the VESC. */
    virtual void toVesc(const std::string &value) = 0;

    /** VESC -> BLE, appends pending VESC data to the given buffer. */
    virtual void fromVesc(std::string &buffer) = 0;

    /** true while the VESC was seen recently on the bus. */
    virtual bool isConnected() = 0;
};

/**
 * Shared polling/retry logic for all transports.
 *
 * Keeps the periodical request scheduling (realtime + balance data), the
 * firmware-version handshake retry and the connected state in one place.
 * Subclasses only implement how frames are read and how requests are sent.
 */
class VescCommunicationBase : public IVescCommunication {
  public:
    explicit VescCommunicationBase(VescData *vescData);
    ~VescCommunicationBase() override = default;

    void loop() final;
    bool isConnected() final { return vescData->connected; }
    /** true as soon as the transport has seen valid data from the VESC. */
    bool isInitialized() const { return initialized; }
    /** polling interval in milliseconds. */
    int getInterval() const { return interval; }

  protected:
    // --- transport hooks, implemented by the concrete transport -------
    /** Consume pending frames/serial data and update vescData/initialized. */
    virtual void readFromVesc() = 0;
    virtual bool sendFirmwareVersionRequest() = 0;
    virtual bool sendRealtimeDataRequest() = 0;
    virtual bool sendFloatPackageDataRequest() = 0;
    /** Return true if a poll must not be sent right now (e.g. proxy busy). */
    virtual bool requestSlotBusy() { return false; }
    /** Optional per-loop finishing work (e.g. debug dump). */
    virtual void afterRead() {}

    void dumpVescValues();

    VescData *vescData;
    VescCommandParser *vescCommandParser;
    bool initialized = false;
    int interval = 500;
    int initRetryCounter = 5;
    unsigned long lastRetry = 0;

  private:
    static const int bufSize = 128;
    char buf[bufSize];
    unsigned long lastDump = 0;
};

/**
 * Creates the transport implementation matching the build configuration.
 */
class VescCommunicationFactory {
  public:
    static VescCommunicationFactory* getInstance();
    static IVescCommunication* createVescCommunication(VescData *vescData);

  private:
    VescCommunicationFactory() = default;
    static VescCommunicationFactory *instance;
};

#endif //__I_VESC_COMMUNICATION_H__
