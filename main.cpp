#include <iostream>
#include "src/Transaction.h"
#include "src/Engine.h"

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::thread;

void initData() {
  Engine::insert("Swagger", "100");
  Engine::insert("Bob", "0");
  Engine::insert("Alice", "10000");
}

// Alice -> 5000 -> Swagger
void T1() {
  TransactionController t;
  if ( !t.BEGIN() ) {
    cerr << "BEGIN() Error!" << endl;
    return;
  }
  string aliceM;
  ExecState s = t.read("Alice", aliceM);
  if ( s == TFailed )
    return;
  int aliceMInt = std::stoi(aliceM);
  aliceMInt -= 5000;
  string newValue = std::to_string(aliceMInt);
  s = t.write("Alice", newValue);
  if ( s == TFailed )
    return;

  string swaggerM;
  s = t.read("Swagger", swaggerM);
  if ( s == TFailed )
    return;
  int SwaggerMInt = std::stoi(swaggerM);
  SwaggerMInt += 5000;
  newValue = std::to_string(SwaggerMInt);
  t.write("Swagger", newValue);
  if ( s == TFailed )
    return;
  t.END();
}


// Bob -> 100 -> Alice
void T2() {
  TransactionController t;
  if ( !t.BEGIN() ) {
    cerr << "BEGIN() Error!" << endl;
    return;
  }
  string BobM;
  ExecState s = t.read("Bob", BobM);
  if ( s == TFailed )
    return;
  int BobMInt = std::stoi(BobM);
  if (BobMInt < 100) {
    t.ABORT();
  }else {
    BobMInt -= 100;
    string newValue = std::to_string(BobMInt);
    s = t.write("Bob", newValue);
    if ( s == TFailed )
      return;

    string AliceM;
    s = t.read("Alice", AliceM);
    if ( s == TFailed )
      return;
    int AliceMInt = std::stoi(AliceM);
    AliceMInt += 100;
    newValue = std::to_string(AliceMInt);
    s = t.write("Alice", newValue);
    if ( s == TFailed )
      return;
    t.END();
  }
}


void T3() {
  TransactionController t;
  if ( !t.BEGIN() ) {
    cerr << "BEGIN() Error!" << endl;
    return;
  }
  string SwaggerM;
  ExecState s = t.read("Swagger", SwaggerM);
  if ( s == TFailed )
    return;
  int SwaggerMInt = std::stoi(SwaggerM);
  SwaggerMInt -= 100;
  string newValue = std::to_string(SwaggerMInt);
  s = t.write("Swagger", newValue);
  if ( s == TFailed )
    return;

  string BobM;
  s = t.read("Bob", BobM);
  if ( s == TFailed )
    return;
  int BobMInt = std::stoi(BobM);
  BobMInt += 100;
  newValue = std::to_string(BobMInt);
  s = t.write("Bob", newValue);
  if ( s == TFailed )
    return;
  t.END();
}


int main() {

  initData();

  //Engine::show();

  thread t1(T1);
  thread t2(T2);
  thread t3(T3);
  t1.join();
  t2.join();
  t3.join();


  Engine::show();

}
