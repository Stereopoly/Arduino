

/**
 * Example of pinging across a mesh network
 *
 * Using this sketch, each node will send a ping to every other node
 * in the network every few seconds. 
 * The RF24Network library will route the message across
 * the mesh to the correct node.
 *
 * This sketch is greatly complicated by the fact that at startup time, each
 * node (including the base) has no clue what nodes are alive.  So,
 * each node builds an array of nodes it has heard about.  The base
 * periodically sends out its whole known list of nodes to everyone.
 *
 * To see the underlying frames being relayed, compile RF24Network with
 * #define SERIAL_DEBUG.
 *
 * The logical node address of each node is set in EEPROM.  The nodeconfig
 * module handles this by listening for a digit (0-9) on the serial port,
 * and writing that number to EEPROM.
 *
 * To use the sketch, upload it to two or more units.  Run each one in
 * turn.  Attach a serial monitor, and send a single-digit address to
 * each.  Make the first one '0', and the following units '1', '2', etc.
 */

#include <avr/pgmspace.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include "nodeconfig.h"
#include "printf.h"
#include <Ethernet.h>

// This is for git version tracking.  Safe to ignore
#ifdef VERSION_H
#include "version.h"
#else
#define __TAG__ "Unknown"
#define SERIAL_DEBUG
#endif

// Webpage setup
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };   //physical mac address
byte ip[] = { 
  192, 168, 1, 178 };                    // browser ip address for connecting to webpage
byte gateway[] = { 
  192, 168, 1, 1 };                   // internet access via router
byte subnet[] = { 
  255, 255, 255, 0 };                  //subnet mask
EthernetServer server(80);            //server port     
String readString;                  // for debugging

String ledOnOff;                      // Status setup
String onOff;
String doorStatus;

int messageToBeSent;         // Declaring variable with holding message
int relay = 7;              // Port relay is connected to
int sent = 2;
uint16_t to;


// nRF24L01(+) radio setup
RF24 radio(8,9);
RF24Network network(radio);

// Our node address
uint16_t this_node;

// Delay manager to send pings regularly
const unsigned long interval = 2000; // ms
unsigned long last_time_sent;

// Array of nodes we are aware of
const short max_active_nodes = 10;
uint16_t active_nodes[max_active_nodes];
short num_active_nodes = 0;
short next_ping_node_index = 0;

// Prototypes for functions to send & handle messages
bool send_T(uint16_t to);
bool send_N(uint16_t to);
void handle_T(RF24NetworkHeader& header);
void handle_N(RF24NetworkHeader& header);
void add_node(uint16_t node);

void setup(void)
{
  
  // Port setup  
  pinMode(4, OUTPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(10, HIGH);
  
  //
  // Print preamble
  //

  Serial.begin(57600);
  printf_begin();
  printf_P(PSTR("\n\rRF24Network/examples/meshping/\n\r"));
  printf_P(PSTR("VERSION: " __TAG__ "\n\r"));

  //
  // Pull node address out of eeprom 
  //

  // Which node are we?
  this_node = nodeconfig_read();

  //
  // Bring up the RF network
  //

  SPI.begin();
  radio.begin();
  network.begin(/*channel*/ 100, /*node address*/ this_node );
  
  //
  // start the Ethernet connection and the server:
  //
  
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  ledOnOff = "Off";
  onOff = "Off";
}

void loop(void)
{
  // Pump the network regularly
  network.update();
  
  EthernetClient client = server.available();
  if (client) {
    while (client.connected()) {   
      if (client.available()) {
        char c = client.read();

        //read char by char HTTP request
        if (readString.length() < 100) {
          //store characters to string
          readString += c;
          //Serial.print(c);
        }

        //if HTTP request has ended
        if (c == '\n') {          
          Serial.println(readString); //print to serial monitor for debuging

          client.println("HTTP/1.1 200 OK"); //send new page
          client.println("Content-Type: text/html");
          client.println();     
          client.println("<HTML>");
          client.println("<HEAD>");
          client.println("<meta name='apple-mobile-web-app-capable' content='yes' />");
          client.println("<meta name='apple-mobile-web-app-status-bar-style' content='black-translucent' />");
          client.println("<meta http-equiv=\"refresh\" content=\"2\">");
          client.println("<link rel='stylesheet' type='text/css' href='http://randomnerdtutorials.com/ethernetcss.css' />");
          client.println("<style>");
          client.println("body {");
          client.println("background-color: lightblue;");
          client.println(" }");
          client.println("</style>");
          client.println("<TITLE>Home Automation</TITLE>");
          client.println("</HEAD>");
          client.println("<BODY>");
          client.println("<H1>Home Control System</H1>");
          client.println("<H3>Wait a second before pressing a button again</H3>");
          client.println("<hr />");
          client.println("<a href=\"/?button3\"\">Check Door Status</a><br />");
          client.println("<br />");  
          client.println("Door is: ");
          client.println(doorStatus);    
          client.println("<br />");  
 //         client.println("<a href=\"/?button1on\"\">Turn On LED</a>");
  //        client.println("<a href=\"/?button1off\"\">Turn Off LED</a><br />");   
  //        client.println("<br />");     
   //       client.println("LED is: ");
   //       client.println(ledOnOff);
          client.println("<br />");
          client.println("<br />");
          client.println("<a href=\"/?button2on\"\">Relay On</a>");
          client.println("<a href=\"/?button2off\"\">Relay Off</a><br />");
          client.println("<br />");
          client.println("Relay is: ");
          client.println(onOff);
          client.println("<br />");
          client.println("<br />");
 //         client.println("<a href=\"/?button3\"\">Check Door Status</a><br />");
//          client.println("<a href=\"/?button3off\"\">Check Door Status</a><br />");
  //        client.println("<br />");
  //        client.println("Door is: ");
   //       client.println(doorStatus);    
          client.println("<p>Created by Oscar Bjorkman</p>");  
          client.println("<br />"); 
          client.println("</BODY>");
          client.println("</HTML>");

          delay(1); // 1 millisecond
          //stopping client
          client.stop();
          //controls the Arduino if you press the buttons
 //         if (readString.indexOf("?button1on") >0){
 //           messageToBeSent = 1;
 //           ledOnOff = "On";
 //           printf("LED ON Pressed \n");
 //           printf("To be sent: %i", messageToBeSent);
 //           to = 02;
 //         }
 //         if (readString.indexOf("?button1off") >0){
 //           messageToBeSent = 0;
 //           ledOnOff = "Off";
 //           printf("LED OFF Pressed \n");
 //           printf("To be sent: %i", messageToBeSent);
 //           to = 02;
  //        }
          // Relay control
          if (readString.indexOf("?button2on") >0){
            digitalWrite(relay, HIGH);      // sends power to relay which creates circuit
            Serial.println("Relay on");
            messageToBeSent = 2;
            onOff = "On";
            to = 05;
          }
          if (readString.indexOf("?button2off") >0){
            digitalWrite(relay, LOW);      // cuts power to relay which opens circuit
            Serial.println("Relay off");
            messageToBeSent = 3;
            onOff = "Off";
            to = 05;
          }
          if (readString.indexOf("?button3") >0){
            messageToBeSent = 9;
            to = 02;
              Serial.println("Check Door");
          }
          //clearing string for next read
          delay(50);
          
          readString=""; 

//          // door status

//          long duration; 
//          long distance;
//          digitalWrite(trigPin, LOW);  
//          delayMicroseconds(2);
//          digitalWrite(trigPin, HIGH);
//          delayMicroseconds(10); 
//          digitalWrite(trigPin, LOW);
//          duration = pulseIn(echoPin, HIGH);
//          distance = (duration/2) / 29.1;
//          if (distance < 4) {  // LED On/Off loop
//            digitalWrite(uled, HIGH); 
//            digitalWrite(uled2, LOW);
//            doorStatus = "Closed";
//          }
//          else {
//            digitalWrite(uled, LOW);
//            digitalWrite(uled2, HIGH);
//            doorStatus = "Open";
//          }
//          delay(500); 

        }
      }
    }
  }

  // Is there anything ready for us?
  while ( network.available() )
  {
    // If so, take a look at it 
    RF24NetworkHeader header;
    network.peek(header);

    // Dispatch the message to the correct handler.
    switch (header.type)
    {
    case 'T':
      handle_T(header);
      break;
    case 'N':
      handle_N(header);
      break;
    default:
      printf_P(PSTR("*** WARNING *** Unknown message type %c\n\r"),header.type);
      network.read(header,0,0);
      break;
    };
  }
  
  int sending;

  // Send a ping to the next node every 'interval' ms
  unsigned long now = millis();
  if ( now - last_time_sent >= interval )
  {
    last_time_sent = now;

    // Who should we send to?
    // By default, send out
    
//    if ( sending == 2) {
//      to = 02;
//      sending = 5; 
//    }
//    else {
//       to = 05;
//       sending = 2; 
//    }
    
//    // Or if we have active nodes,
//    if ( num_active_nodes )
//    {
//      // Send to the next active node
//      to = active_nodes[next_ping_node_index++];
//
//      // Have we rolled over?
//      if ( next_ping_node_index > num_active_nodes )
//      {
//        // Next time start at the beginning
//        next_ping_node_index = 02;
//
//        // This time, send to node 02.
//        to = 05;
//      }
//    }

    bool ok;

    // nodes send a 'T' ping
    if ( this_node == 00 && to == 02 )
      ok = send_T(to);
    
    if ( this_node == 00 && to == 05 )
      ok = send_T(to);

    // Base node sends the current active nodes out
    if ( this_node == 00) {
      ok = send_N(to);
    }
    
    // Notify us of the result
    if (ok)
    {
      printf_P(PSTR("%lu: APP Send ok\n\r"),millis());
    }
    else
    {
      printf_P(PSTR("%lu: APP Send failed\n\r"),millis());

      // Try sending at a different time next time
      last_time_sent -= 100;
    }
  }

  // Listen for a new node address
  nodeconfig_listen();
}

// End of main loop ************************************************************** //

/**
 * Send a 'T' message
 */
bool send_T(uint16_t to)
{
  RF24NetworkHeader header(/*to node*/ to, /*type*/ 'T' /*Time*/);

  // The 'T' message that contains info on what to control
  unsigned long message = messageToBeSent;                                       // message  being  sent  between  nodes
   
  printf_P(PSTR("---------------------------------\n\r"));
  printf_P(PSTR("%lu: APP Sending %lu to 0%o...\n\r"),millis(),message,to);
  return network.write(header,&message,sizeof(unsigned long));
}

/**
 * Send an 'N' message, the active node list
 */
bool send_N(uint16_t to)
{
  RF24NetworkHeader header(/*to node*/ to, /*type*/ 'N' /*Time*/);

  printf_P(PSTR("---------------------------------\n\r"));
  printf_P(PSTR("%lu: APP Sending active nodes to 0%o...\n\r"),millis(),to);
  return network.write(header,active_nodes,sizeof(active_nodes));
}

/**
 * Handle a 'T' message
 *
 * Add the node to the list of active nodes
 */
void handle_T(RF24NetworkHeader& header)
{
  // The 'T' message is receieved and put into this variable
  unsigned long message;
  network.read(header,&message,sizeof(unsigned long));
  printf_P(PSTR("%lu: APP Received %lu from 0%o\n\r"),millis(),message,header.from_node);

  // If this message is from ourselves or the base, don't bother adding it to the active nodes.
  if ( header.from_node != this_node || header.from_node > 00 ) {
    add_node(header.from_node);
  }
  // If the message is from ourselves we don't want it
  if ( header.from_node != this_node) {
    printf("Self");
  }
  if( message == 1 ) {    // some testing code to turn on LED if number 1 is received
    digitalWrite(4, HIGH);
    printf("LED on \n"); 
  }          // based on what is receieved so this stuff ::::
  if(message == 0) {
    digitalWrite(4, LOW);
    printf("LED off \n");
  }
  if(message == 2) {              // Whether or not relay has been activated or not detection
    digitalWrite(relay, HIGH);
    printf("Relay On \n"); 
  }  
  if(message == 3) {
    digitalWrite(relay, LOW);
    printf("Relay Off \n"); 
  }
  else {                             // If there is an error is transmission
    printf("Message was invalid");
  }  
}

/**
 * Handle an 'N' message, the active node list
 */
void handle_N(RF24NetworkHeader& header)
{
  static uint16_t incoming_nodes[max_active_nodes];

  network.read(header,&incoming_nodes,sizeof(incoming_nodes));
  printf_P(PSTR("%lu: APP Received nodes from 0%o\n\r"),millis(),header.from_node);

  int i = 0;
  while ( i < max_active_nodes && incoming_nodes[i] > 00 )
    add_node(incoming_nodes[i++]);
}

/**
 * Add a particular node to the current list of active nodes
 */
void add_node(uint16_t node)
{
  // Do we already know about this node?
  short i = num_active_nodes;
  while (i--)
  {
    if ( active_nodes[i] == node )
      break;
  }
  // If not, add it to the table
  if ( i == -1 && num_active_nodes < max_active_nodes )
  {
    active_nodes[num_active_nodes++] = node; 
    printf_P(PSTR("%lu: APP Added 0%o to list of active nodes.\n\r"),millis(),node);
  }
}

// vim:ai:cin:sts=2 sw=2 ft=cpp

