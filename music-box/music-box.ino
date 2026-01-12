char *config = "eb,66.";
char *melody =
  R"(b-4-8 g-5-2 f-5-8 g-5-8 f-5d4 e-5-4
b-4-8 g-5-4 c-5-40 c#5-* c-5-* b#4-* c-5-* c-6-4 g-5-8 b-5d4 a-5-4 g-5-8
f-5d4 g-5-4 d-5-8 e-5d4 c-5d4 b-4-8 d-6-* c-6-* b-5-16 a-5-* g-5-* a-5-* c-5-* d-5-* e-5d4 r---4
b-4-8 g-5d4 f-5-16 g-5-16 f-5-48 g-5-* f-5-* e#5-16 f-5-* g-5-* f-5-8 e-5-4 e-5-16 f-5-* e-5-48 f-5-* e-5-* d-5-16 e-5-* f-5-*
g-5-16 b#4-* c-5-* db5-* c-5-* f-5-* e#5-* a-5-* g-5-* db6-* c-6-* g-5-* b-5d4 a-5-4 g-5-8
f-5-8 f-5-32 g-5-* f-5-* g-5-* f-5-* g-5-* e#5-* f-5-* g-5-16 r---16 g-5-8 d-5-8 e-5d4 c-5d4
b-4-8 d-6-* c-6-* b-5-16 a-5-* g-5-* a-5-* a-5-64 c-5d32 d-5-16 e-5d4)";

/*
  config file format:
  c#,60. <-- tempo             
  /\__ key signature (relative major)
  comma separated, period terminated
  
  track format:
  c-7-4 d-7-4 e-7-4 ... (notes separated by one space or one \n)

accidental (b/-/#)   interval (multiple digits supported, '*' for same as previous note)
                 \    /
    note format: c#6d4 
               /    \ \------------------\   
           note    octave [a4 = 440Hz]   dotted
 
 this is a c-sharp-6 quarter note.
 
 more examples:
 d-4-8 --> d4 eighth note
 bb2-2 --> b-flat-2 half note
 a-4-4 --> a4 quarter note
 e-7-16 --> e7 16th note
 f-5-* --> f5 16th note (same as above)
 g-3d2 --> g3 dotted half note

*/

char key[2];
int tempo;

typedef struct Track {
  char *notes;
  int pin;
  int n;             // which character of file are we reading right now;
  int lastInterval;  // the speed (fraction) of the last note, useful for grouping
  bool complete;     // has the track finished playing
};

// settings
int pin = 3;
Track track1 = { melody, pin, 0, 1, false };
#define SERIAL false

void pinInit(Track track) {
  pinMode(track.pin, OUTPUT);
}

void setup() {
  #if SERIAL
  Serial.begin(115200);
  while (!Serial) {
    delay(100);
  }
  #endif
  pinInit(track1);
  #if SERIAL
  Serial.println("welcome to the music box!");
  #endif

  int i = 0;
  while (config[i] != ',') {
    if (i < 2) {
      key[i] = config[i];  // read first two characters of config file to key[]
    }
    i++;
  }
  i++;  // next field
  char tempoStr[10] = { 0 };
  int j = 0;
  while (config[i] != '.') {
    if (j < 10) {
      tempoStr[j] = config[i];
    }
    i++;
    j++;
  }
  tempo = atoi(tempoStr);
  #if SERIAL
  Serial.print("key: ");
  Serial.println(key);
  Serial.print("tempo: ");
  Serial.print(tempo);
  Serial.println(" bpm");
  #endif
}

void loop() {
  nextNote(&track1);
}

void nextNote(Track *track) {
  if (track->complete) {
    delay(1);
    return;
  }

  char letter = track->notes[track->n];
  char accidental = track->notes[track->n + 1];
  int octave = track->notes[track->n + 2] - 48;
  bool dotted = track->notes[track->n + 3] == 'd';

  // parse interval length
  int i = track->n + 4;
  char intervalStr[10] = { 0 };
  int j = 0;
  while (!(track->notes[i] == ' ' || track->notes[i] == '\n') && j < 10) {
    intervalStr[j] = track->notes[i];
    i++;
    j++;
  }
  int interval = 0;
  if (!strcmp(intervalStr, "*")) {
    interval = track->lastInterval;
  } else {
    interval = atoi(intervalStr);
    track->lastInterval = interval;
  }

  #if SERIAL
  Serial.print(letter);
  Serial.print(" ");
  Serial.print(accidental);
  Serial.print(" ");
  Serial.print(octave);
  Serial.print(" ");
  Serial.print(dotted ? "d" : "-");
  Serial.print(" ");
  Serial.println(interval);
  #endif

  play(track->pin, letter, accidental, octave, dotted, interval);

  if (track->n + 1 >= strlen(track->notes) - 1) {
    track->complete = true;
    #if SERIAL
    Serial.println("track done!");
    #endif
  } else {
    track->n += strlen(intervalStr) + 4 + 1;
  }
  return;
}

void play(int pin, char letter, char accidental, int octave, bool dotted, int interval) {
  double frequency = 0;
  if (letter != 'r') {
    frequency = pow(1.059463, getAOffset(letter, accidental)) * 440 * pow(2, octave - 4);
  }
  int duration = (int)(60000 / tempo / interval * 4 * (dotted ? 1.5 : 1));
  tone(pin, frequency, duration);
  delay(duration);
}

int getAOffset(char letter, char accidental) {
  int noteOffset = 0;
  int accidentalOffset = 0;
  int signatureOffset = 0;

  switch (accidental) {
    case '#':
      accidentalOffset = 1;
      break;
    case 'b':
      accidentalOffset = -1;
      break;
  }

  switch (letter) {
    case 'c':
      noteOffset = -9;
      break;
    case 'd':
      noteOffset = -7;
      break;
    case 'e':
      noteOffset = -5;
      break;
    case 'f':
      noteOffset = -4;
      break;
    case 'g':
      noteOffset = -2;
      break;
    case 'a':
      noteOffset = 0;
      break;
    case 'b':
      noteOffset = 2;
      break;
  }

  if (!strcmp(key, "c")) {
    // no accidentals.
  } else if (!strcmp(key, "db")) {
    switch (letter) {
      case 'd':
      case 'e':
      case 'g':
      case 'a':
      case 'b':
        signatureOffset = -1;
        break;
    }
  } else if (!strcmp(key, "d")) {
    switch (letter) {
      case 'f':
      case 'c':
        signatureOffset = 1;
        break;
    }
  } else if (!strcmp(key, "eb")) {
    switch (letter) {
      case 'e':
      case 'a':
      case 'b':
        signatureOffset = -1;
        break;
    }
  } else if (!strcmp(key, "e")) {
    switch (letter) {
      case 'f':
      case 'c':
      case 'g':
      case 'd':
        signatureOffset = -1;
        break;
    }
  } else if (!strcmp(key, "f")) {
    switch (letter) {
      case 'b':
        signatureOffset = -1;
        break;
    }
  } else if (!strcmp(key, "gb")) {
    switch (letter) {
      case 'b':
      case 'e':
      case 'a':
      case 'd':
      case 'c':
      case 'g':
        signatureOffset = -1;
        break;
    }
  } else if (!strcmp(key, "g")) {
    switch (letter) {
      case 'f':
        signatureOffset = 1;
        break;
    }
  } else if (!strcmp(key, "ab")) {
    switch (letter) {
      case 'b':
      case 'e':
      case 'a':
      case 'd':
        signatureOffset = 1;
        break;
    }
  } else if (!strcmp(key, "a")) {
    switch (letter) {
      case 'f':
      case 'c':
      case 'g':
        signatureOffset = 1;
        break;
    }
  } else if (!strcmp(key, "bb")) {
    switch (letter) {
      case 'b':
      case 'e':
        signatureOffset = -1;
        break;
    }
  } else if (!strcmp(key, "b")) {
    switch (letter) {
      case 'f':
      case 'c':
      case 'g':
      case 'd':
      case 'a':
        signatureOffset = 1;
        break;
    }
  } else {
    #if SERIAL
    Serial.print("error, did not recognize key signature \"");
    Serial.print(key);
    Serial.println("\"");
    #endif
  }

  return noteOffset + accidentalOffset + signatureOffset;
}