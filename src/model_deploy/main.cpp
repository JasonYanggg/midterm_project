#include "mbed.h"
#include "uLCD_4DGL.h"
#include "accelerometer_handler.h"
#include "config.h"
#include "magic_wand_model_data.h"
#include "DA7212.h"
#include <cmath>

#include "tensorflow/lite/c/common.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#define N (3) // number of songs
#define bufferLength (32)

DA7212 audio;
int16_t waveform[kAudioTxBufferSize];
uLCD_4DGL uLCD(D1, D0, D2);
int show;
int modes;
int songs;
char name[3][20] = {"little star", "little bee", "do"};
Thread t;
Thread song_t;
// Thread disp_t;
EventQueue queue(32 * EVENTS_EVENT_SIZE);
EventQueue song_queue(32 * EVENTS_EVENT_SIZE);
// EventQueue disp_queue(32 * EVENTS_EVENT_SIZE);
InterruptIn pause(SW2);
// DigitalIn pause(SW2);
InterruptIn button(SW3);
int idC = 0;
Serial pc(USBTX, USBRX);
int length;
int song[100];
int noteLength[100];
int finish = 0;
char serialInBuffer[bufferLength];
int serialCount = 0;
int gesture;
int enterrr = 0;
int index_note;
int len_note;
bool play = false;

// Create an area of memory to use for input, output, and intermediate arrays.
// The size of this will depend on the model you're using, and may need to be
// determined by experimentation.
constexpr int kTensorArenaSize = 60 * 1024;
uint8_t tensor_arena[kTensorArenaSize];
// Set up logging.
static tflite::MicroErrorReporter micro_error_reporter;
tflite::ErrorReporter* error_reporter = &micro_error_reporter;
// Map the model into a usable data structure. This doesn't involve any
// copying or parsing, it's a very lightweight operation.
const tflite::Model* model = tflite::GetModel(g_magic_wand_model_data);
static tflite::MicroOpResolver<6> micro_op_resolver;
  
                                // Build an interpreter to run the model with
static tflite::MicroInterpreter static_interpreter(
    model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
tflite::MicroInterpreter* interpreter = &static_interpreter;

// Obtain pointer to the model's input tensor
TfLiteTensor* model_input;
int input_length;
TfLiteStatus setup_status = SetupAccelerometer(error_reporter);
// Whether we should clear the buffer next time we fetch data
bool should_clear_buffer = false;

// play note
// void playNote(void)
// {
//   for (int i = 0; i < kAudioTxBufferSize; i++)
//   {
//     waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / song[index_note])) * ((1<<16) - 1));
//   }
//   // the loop below will play the note for the duration of 1s
//   for(int j = 0; j < kAudioSampleFrequency / kAudioTxBufferSize; ++j)
//   {
//     audio.spk.play(waveform, kAudioTxBufferSize);
//   }
//   len_note--;
//   if (len_note == 0) {
//     index_note++;
//     index_note %= length;
//     len_note = noteLength[index_note];
//   }
// }
void playNote(int freq)
{
  for (int i = 0; i < kAudioTxBufferSize; i++)
  {
    waveform[i] = (int16_t) (sin((double)i * 2. * M_PI/(double) (kAudioSampleFrequency / freq)) * ((1<<16) - 1));
  }
  // the loop below will play the note for the duration of 1s 
  audio.spk.play(waveform, kAudioTxBufferSize);
}

// play song and stop song
void play_song(void)
{
  int i;
  while (play) {
    for (i = 0; play && i < length; i++) {
      int j = noteLength[i];
      while (play && j--) {
        for (int k = 0; k < kAudioSampleFrequency / kAudioTxBufferSize; k++) {
          song_queue.call(playNote, song[i]);
        }
        if (j >= 0) wait(1.0);
      }
    }
    wait(1.0);
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

void get_gest(void)
{
  bool got_data = false;
    
  // The gesture index of the prediction
  int gesture_index;  // 0 -> down, 1 -> up

  while (!finish) {
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

    // Produce an output
    if (gesture_index < label_num) {
      // error_reporter->Report(config.output_message[gesture_index]);
      // return gesture_index;
      gesture = gesture_index + 1;
      break;
    }
  }
}

void display(void)
{
  uLCD.cls();
  if (show == 0) {
    uLCD.color(BLUE);
    uLCD.printf("User manual\n\n");
    uLCD.printf("1. Press SW2 to \n  select mode\n\n");
    uLCD.printf("2. Move clockwise\n  to scroll down\n\n");
    uLCD.printf("3. Move counter \n  clockwise to\n  scroll up\n\n");
    uLCD.printf("4. Press SW3 to \n  to select");
  }
  else if (show == 1) {
    uLCD.color(BLUE);
    uLCD.printf("\n  forward\n");
    uLCD.printf("\n  backward\n");
    uLCD.printf("\n  change songs\n");
    uLCD.locate(0, 2 * modes - 1);
    uLCD.printf(">");
  }
  else if (show == 2) {
    uLCD.color(BLUE);
    uLCD.printf("\n  %s\n", name[0]);
    uLCD.printf("\n  %s\n", name[1]);
    uLCD.printf("\n  %s\n", name[2]);
    uLCD.locate(0, 2 * songs - 1);
    uLCD.printf(">");
  }
  else if (show == 3) {
    uLCD.circle(64, 50, 40, WHITE);
    uLCD.triangle(45, 25, 45, 75, 95, 50, WHITE);
    uLCD.color(BLUE);
    uLCD.locate(0, 13);
    if (!finish) {
      uLCD.printf("Loading:\n  %s", name[songs - 1]);
    }
    else {
      uLCD.printf("Now playing:\n  %s", name[songs - 1]);  
    }
  }
}
// song selection
void song_selection(void)
{
  int j = 0;

  // select song
  if (modes == 1) {
    songs++; 
    if (songs > N) {
      songs = 1;
    }    
  }
  else if (modes == 2) {
    songs--;
    if (songs == 0) {
      songs = N;
    }
  }
  else if (modes == 3) {
    finish = 0;
    show = 2;
    display();
    while (!finish) {
      gesture = 0;
      get_gest();
      if (gesture == 1) {
        if (songs < N) {
          songs++;
          display();
        }
      }
      else if (gesture == 2) {
        if (songs > 1) {
          songs--;
          display();
        }
      }
    }
  }

  // read in selected song
  finish = 0;
  serialCount = 0;
  show = 3;
  display();
  pc.printf("%d\r\n", songs);
  while (!finish) {
    if (pc.readable()) {
      serialInBuffer[serialCount] = pc.getc();
      serialCount++;
      if (serialCount == 5) {
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
    serialInBuffer[serialCount] = pc.getc();
      serialCount++;
      if (serialCount == 5) {
        serialInBuffer[serialCount] = '\0';
        noteLength[j] = (int)atoi(serialInBuffer);
        serialCount = 0;
        j++;
      }
  }
  display();
}

void stop_play_song(void)
{
  // queue.cancel(idC);
  play = false;
  for (int i = 0; i < kAudioTxBufferSize; i++)
  {
    waveform[i] = (int16_t) 0;
  }
  // the loop below will play the note for the duration of 1s 
  audio.spk.play(waveform, kAudioTxBufferSize);
}

// mode selection
void mode_selection(void)
{
  modes = 1;
  finish = 0;
  show = 1;
  display();
  while (!finish) {
    gesture = 0;
    get_gest();
    if (gesture == 2) {
      if (modes < 3) {
        modes++;
        display();
      }
    }
    else if (gesture == 1) {
      if (modes > 1) {
        modes--;
        display();
      }
    }
    wait(0.1);
  }
  song_selection();
  // index_note = 0;
  // len_note = noteLength[index_note];
  // while (finish) {
  // idC = queue.call_every(1000, playNote);
    // wait(1.0);
  // }
  play = true;
  play_song();
}

// when select is pressed
void button_press(void) {finish = !finish;}

int main(int argc, char* argv[]) {
  show = 0;
  modes = 1;
  songs = 1;
  t.start(callback(&queue, &EventQueue::dispatch_forever));
  song_t.start(callback(&song_queue, &EventQueue::dispatch_forever));
  // disp_t.start(callback(&disp_queue, &EventQueue::dispatch_forever));
  // disp_queue.call(get_gest);
  pause.rise(queue.event(mode_selection));
  pause.fall(song_queue.event(stop_play_song));
  button.fall(&button_press);
  display();
  
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

 // Allocate memory from the tensor_arena for the model's tensors
interpreter->AllocateTensors();

model_input = interpreter->input(0);
  
if ((model_input->dims->size != 4) || (model_input->dims->data[0] != 1) ||
    (model_input->dims->data[1] != config.seq_length) ||
    (model_input->dims->data[2] != kChannelNumber) ||
    (model_input->type != kTfLiteFloat32)) {
  error_reporter->Report("Bad input tensor parameters in model");
  return -1;
}

input_length = model_input->bytes / sizeof(float);


if (setup_status != kTfLiteOk) {
  error_reporter->Report("Set up failed\n");
  return -1;
}
// error_reporter->Report("Set up successful...\n");

}