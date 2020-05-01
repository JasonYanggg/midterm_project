#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"
#include "mbed.h"
#include <cmath>
#include "DA7212.h"
#include "uLCD_4DGL.h"

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#define N 3 // number of songs

uLCD_4DGL uLCD(D1, D0, D2);
DA7212 audio;
int16_t waveform[kAudioTxBufferSize];
int modes;  // 1 -> forward, 2 -> backward, 3 -> change songs
int songs;  // 1, 2, 3
string name[3] = {"little star", "little bee", "two tigers"};
InterruptIn pause(SW2);
InterruptIn select(SW3);
EventQueue queue(32 * EVENTS_EVENT_SIZE);
EventQueue song_queue(32 * EVNETS_EVENT_SIZE);
Thread t;
Thread song_t;
Thread disp_t;
int idC = 0;
Serial pc(USBTX, USBRX);
int length;
int song[100];
int noteLength[100];
// The gesture index of the prediction
int gesture_index;  // 0 -> down, 1 -> up
int finish = 0;
int show = 0; // 0 -> user guide, 1 -> mode selection, 2 -> song selection, 3 -> play song

// mode selection
void mode_selection(void)
{
  song_queue.cancel(idC);
  modes = 1;
  finish = 0;
  show = 1;
  while (!finish) {
    if (gesture_index == 1) {
      if (modes < 3)
        modes++;
    }
    else if (gesture_index == 2) {
      if (modes > 1)
        modes--;
    }
  }
  song_selection();
  song_queue.call(play_song);
}

// when select is pressed
void button_press(void) {finish = !finish;}

// song selection
void song_selection(void)
{
  char serialInBuffer[bufferLength];
  int serialCount = 0;
  int i = 0, j = 0;

  songs = 1;
  // select song
  if (modes == 1) {
    songs--;
    if (songs == 0) {
      songs == N;
    }
  }
  else if (modes == 2) {
    songs++;
    if (songs > N) {
      songs = 0;
    }
  }
  else if (modes == 3) {
    finish = 0;
    show = 2;
    while (!finish) {
      if (gesture_index == 1) {
        if (songs < N) 
          songs++;
      }
      else if (gesture_index == 2) {
        if (songs > 1) 
          songs--;
      }
    }
  }

  // read in selected song
  finish = 0;
  show = 3;
  pc.write(songs);
  while (!finish) {
    if (pc.readable()) {
      serialInBuffer[serialCount] = pc.getc();
      serialCount++;
      if (serialCount == 3) {
        serialInBuffer[serialCount] = '\0';
        length = (int)atoi(serialInBuffer);
        serialCount = 0;
        finish = 1;
      }
    }
  }
  while (j < length) {
    if (pc.readable()) {
      serialInBuffer[serialCount] = pc.getc();
      serialCount++;
      if (serialCount == 5) {
        serialInBuffer[serialCount] = '\0';
        song[j] = (int)atoi(serialInBuffer);
        serialCount = 0;
        j++;
      }
    }
  }
  j = 0;
  while (j < length) {
    if (pc.readable()) {
      noteLength[j] = pc.getc() - '0';
      j++;
    }
  }
}

// play note
void playNote(int freq)
{
  for (int i = 0; i < kAudioTxBufferSize; i++)
  {
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  // the loop below will play the note for the duration of 1s
  for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j)
  {
    audio.spk.play(waveform, kAudioTxBufferSize);
  }
}

// play song and stop song
void play_song(void)
{
  int i = 0, j = 0;
  
  while (true) {
    i = noteLength[j];
    while (i > 0) {
      playNote(song[j]);
      wait(1.0);
      i--;
    }
    j++;
    if (j == length) {
      j = 0;
    }
  } 
}

// display on uLCD
// show:
// 0 -> user guide
// 1 -> mode selection
// 2 -> song selection
// 3 -> play song
void display(void)
{
  uLCD.background_color(BLACK);
  uLCD.textbackground_color(BLACK);
  while (true) {
    if (show == 0) {
      uLCD.cls();
      uLCD.color(YELLOW);
      uLCD.locate(1, 2);
      uLCD.printf("Press SW2 to select mode");
    }
    else if (show == 1) {
      uLCD.cls();
      uLCD.color(YELLOW);
      uLCD.printf("  forward\n");
      uLCD.printf("  backward\n");
      uLCD.printf("  change songs\n");
      uLCD.locate(modes, 1);
      uLCD.printf(">");
    }
    else if (show == 2) {
      uLCD.cls();
      uLCD.color(YELLOW);
      uLCD.printf("  %s\n", name[0]);
      uLCD.printf("  %s\n", name[1]);
      uLCD.printf("  %s\n", name[2]);
      uLCD.locate(songs, 1);
      uLCD.printf(">");
    }
    else if (show == 3) {
      uLCD.cls();
      uLCD.circle(1, 1, 30, WHITE);
      uLCD.triangle(20, 20, 20, 60, 80, 40, WHITE);
      uLCD.color(YELLOW);
      uLCD.locate(1, 2);
      uLCD.printf("%s", name[songs - 1]);
    }
  }
}

// Return the result of the last prediction
int PredictGesture(float* output) {
  // How many times the most recent gesture has been matched in a row
  static int continuous_count = 0;
  // The result of the last prediction
  static int last_predict = -1;

  // Find whichever output has a probability > 0.8 (they sum to 1)
  int this_predict = -1;
  for (int i = 0; i < label_num; i++) {
    if (output[i] > 0.8) this_predict = i;
  }

  // No gesture was detected above the threshold
  if (this_predict == -1) {
    continuous_count = 0;
    last_predict = label_num;
    return label_num;
  }

  if (last_predict == this_predict) {
    continuous_count += 1;
  } else {
    continuous_count = 0;
  }
  last_predict = this_predict;

  // If we haven't yet had enough consecutive matches for this gesture,
  // report a negative result
  if (continuous_count < config.consecutiveInferenceThresholds[this_predict]) {
    return label_num;
  }
  // Otherwise, we've seen a positive result, so clear all our variables
  // and report it
  continuous_count = 0;
  last_predict = -1;
  return this_predict;
}

int main(int argc, char* argv[]) {

  t.start(callback(&queue, &EventQueue::dispatch_forever));
  song_t.start(callback(&song_queue, &EventQueue::dispatch_forever));
  disp_t.start(display);
  pause.fall(queue.event(mode_selection));
  select.fall(&button_press);
  modes = 0;
  songs = 0;

  // Create an area of memory to use for input, output, and intermediate arrays.
  // The size of this will depend on the model you're using, and may need to be
  // determined by experimentation.
  constexpr int kTensorArenaSize = 60 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];

  // Whether we should clear the buffer next time we fetch data
  bool should_clear_buffer = false;
  bool got_data = false;

  // Set up logging.
  static tflite::MicroErrorReporter micro_error_reporter;
  tflite::ErrorReporter* error_reporter = &micro_error_reporter;

  // Map the model into a usable data structure. This doesn't involve any
  // copying or parsing, it's a very lightweight operation.
  const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return -1;
  }

  // Pull in only the operation implementations we need.
  // This relies on a complete list of all the ops needed by this graph.
  // An easier approach is to just use the AllOpsResolver, but this will
  // incur some penalty in code space for op implementations that are not
  // needed by this graph.
  static tflite::MicroOpResolver<6> micro_op_resolver;
  micro_op_resolver.AddBuiltin(
      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_MAX_POOL_2D,
                               tflite::ops::micro::Register_MAX_POOL_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
                               tflite::ops::micro::Register_CONV_2D());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_FULLY_CONNECTED,
                               tflite::ops::micro::Register_FULLY_CONNECTED());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
                               tflite::ops::micro::Register_SOFTMAX());
  micro_op_resolver.AddBuiltin(tflite::BuiltinOperator_RESHAPE,
                               tflite::ops::micro::Register_RESHAPE(), 1);

  // Build an interpreter to run the model with
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  tflite::MicroInterpreter* interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors
  interpreter->AllocateTensors();

  // Obtain pointer to the model's input tensor
  TfLiteTensor* model_input = interpreter->input(0);
  if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
      (model_input->dims->data[1] != config.seq_length) ||
      (model_input->dims->data[2] != kChannelNumber) ||
      (model_input->type != kTfLiteFloat32)) {
    error_reporter->Report("Bad input tensor parameters in model");
    return -1;
  }

  int input_length = model_input->bytes / sizeof(float);

  TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
  if (setup_status != kTfLiteOk) {
    error_reporter->Report("Set up failed\n");
    return -1;
  }
  error_reporter->Report("Set up successful...\n");

  while (true) {
    // Attempt to read new data from the accelerometer
    got_data = ReadAccelerometer(error_reporter, model_input->data.f,
                                 input_length, should_clear_buffer);

    // If there was no new data,
    // don't try to clear the buffer again and wait until next time
    if (!got_data) {
      should_clear_buffer = false;
      continue;
    }

    // Run inference, and report any error
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
      error_reporter->Report("Invoke failed on index: %d\n", begin_index);
      continue;
    }

    // Analyze the results to obtain a prediction
    gesture_index = PredictGesture(interpreter->output(0)->data.f);
    // Clear the buffer next time we read data
    should_clear_buffer = gesture_index < label_num;
  }
}
