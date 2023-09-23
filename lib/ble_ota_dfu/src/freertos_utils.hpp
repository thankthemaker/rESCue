/* Copyright 2022 Vincent Stragier */
#pragma once
#ifndef SRC_FREERTOS_UTILS_HPP_
#define SRC_FREERTOS_UTILS_HPP_

#include <Arduino.h>

// Initialize the queue
#define initialize_queue(queue, T, initial_value, size)                        \
  _initialize_queue<T>((queue), (#queue), (size), (initial_value))
#define initialize_empty_queue(queue, T, size)                                 \
  _initialize_queue<T>((queue), (#queue), (size))

template <typename T>
void _initialize_queue(QueueHandle_t *queue, const char *queue_name,
                       size_t size, T *initial_value = nullptr) {
  *queue = xQueueCreate(size, sizeof(T));

  Serial.println("Creating the queue");
  // Init start update queue
  if (*queue == NULL || *queue == nullptr) {
    Serial.println("Error creating the queue");
  }

  if (initial_value == nullptr || initial_value == NULL) {
    return;
  }

  if (!xQueueSend(*queue, initial_value, portMAX_DELAY)) {
    Serial.println("Error initializing the queue");
  }
}

// Create a task and assert it's creation
BaseType_t xTaskCreatePinnedToCoreAndAssert(
    TaskFunction_t pvTaskCode, const char *pcName, uint32_t usStackDepth,
    void *pvParameters, UBaseType_t uxPriority, TaskHandle_t *pvCreatedTask,
    BaseType_t xCoreID);

#endif /* SRC_FREERTOS_UTILS_HPP_ */
