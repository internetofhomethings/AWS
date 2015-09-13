<h2><strong>NOTE: This repository contains some preliminary work which has been shared here for some collaboration.
For a completed example using the AWS API, please see my <a href="https://github.com/internetofhomethings/AWS-ESP8266-API">AWS-ESP8266-API</a> repository.</strong></h2>
<hr />
<h2><strong>AWS interface for ESP8266 Development Software</strong></h2>
This project attempts to interface the ESP8266 with AWS DynamoDB using the Arduino IDE platform.

Setup:

1. Copy the ASW folder to your Arduino "../libraries" folder.
2. Copy the WiFiWebServer9703-aws folder to your Arduino IDE sketchbook folder.

Operation:

The ESP8266 sketch connects to your local Wifi and starts a small server that waits for requests.

The current settings (change if needed in the sketch to match your configuration):

ESP8266 Static IP: 192.168.0.174
ESP8266 Server Port: 9703
Router IP: 192.168.0.1

Serial port baud: 74880 bps (Use ESPlorer to monitor ESP8266 serial port)

AWS Settings:

const char* regionName = "us-west-2";
const char* serviceName = "dynamodb/aws4_request";
const char* domainName = "dynamodb.us-west-2.amazonaws.com";
const char* AccessKeyId= "YourAWS_AccessKeyId";
const char* SecretAccessKey= "YourAWS_SecretAccessKey";

To initiate the hard-coded request to AWS, enter the following in a webbrowser on a computer that is connected to the ESP8266 via a serial port:

http://192.168.0.174:9703/?request=GetSensors

The browser will receive a JSON formated response similar to:

{
"awsKeyID":"YourAWSKey",
"awsSecKey":"YourAWSsecretKey",
"payloadHash":"7b80fb15dd80c5fecd6c575463166f6ba3ae1df84765bdf86035718b30a22bd1",
"canonicalRequestHash":"d202fd0eddcbcc001121861b892da5530d2c3e579a94166022ba8d6db9cf19dc",
"kSecret":"AWS4SRlA30FiXvbLsb8nE8qDmhy0e7RksfTHpQ1QP3Py",
"kDate":"27d2804cf0e73029445a97be9e8a2cac334698ee23cb913e288480b13892cade",
"kRegion":"5491949909b38c9e31162f2fb28b95a13ddbd9239530efa0cec4865133fbd6a3",
"kService":"c752128683a1dfd8d7b7aad64ad8a7fcc08f101ee3383c421ab90b38d5edd7c7",
"kSigning":"3f491b60334015efc00ceaa74e64d7bd02fdf8906aeefa1a5c3e11eeb7ea32f6",
"kSignature":"1533a3d7ecde3ac0b854c705fa47487bc587e8afa5c03ba0d498501a864db81b",
"SYS_Heap":"20064",
"SYS_Time":"26"
}

The serial port output from this request will look similar to this:


Recv http: GET /?request=GetSensors HTTP/1.1


Canonical Request String:

POST
/

host:dynamodb.us-west-2.amazonaws.com
x-amz-date:20150727T180750Z
x-amz-target:DynamoDB_20120810.GetItem

host;x-amz-date;x-amz-target
7b80fb15dd80c5fecd6c575463166f6ba3ae1df84765bdf86035718b30a22bd1


Request to AWS:


POST / HTTP/1.1
Host: dynamodb.us-west-2.amazonaws.com
x-amz-date: 20150727T180750Z
x-amz-target: DynamoDB_20120810.GetItem
Authorization: AWS4-HMAC-SHA256 Credential=AKIAINLZZMKKNYHBI3IQ/20150727/us-west-2/dynamodb/aws4_request,SignedHeaders=host;x-amz-date;x-amz-target,Signature=1533a3d7ecde3ac0b854c705fa47487bc587e8afa5c03ba0d498501a864db81b
Date: Mon, 27 Jul 2015 18:07:50 GMT
content-type: application/x-amz-json-1.0
content-length: 126
connection: Keep-Alive

{
    "TableName": "AWSArduinoSDKDemo",
    "Key": {
        "DemoName": {"S":"Colors"},
        "id": {"S":"1"}
    }
}


Reply from AWS:


HTTP/1.1 400 Bad Request
x-amzn-RequestId: ST8N1IN0G5AK3LT8R7T84NMTH3VV4KQNSO5AEMVJF66Q9ASUAAJG
x-amz-crc32: 1263660377
Content-Type: application/x-amz-json-1.0
Content-Length: 718
Date: Mon, 27 Jul 2015 18:07:51 GMT

{"__type":"com.amazon.coral.service#InvalidSignatureException","message":"The request signature we calculated does not match the signature you provided. Check your AWS Secret Access Key and signing method. Consult the service documentation for details.\n\nThe Canonical String for this request should have been\n'POST\n/\n\nhost:dynamodb.us-west-2.amazonaws.com\nx-amz-date:20150727T180750Z\nx-amz-target:DynamoDB_20120810.GetItem\n\nhost;x-amz-date;x-amz-target\n7b80fb15dd80c5fecd6c575463166f6ba3ae1df84765bdf86035718b30a22bd1'\n\nThe String-to-Sign should have been\n'AWS4-HMAC-SHA256\n20150727T180750Z\n20150727/us-west-2/dynamodb/aws4_request\nd202fd0eddcbcc001121861b892da5530d2c3e579a94166022ba8d6db9cf19dc'\n"}
Ending it: Client disconnected

Note: when no request are beeing made, the ESP8266 rotates between 8 selections. 
This is left-over code from a prior project that monitored 8 analog sensors. 
It is currently being used simple to confirm that the ESP8266 is alive.

The AWS Dynamodb used in the example was based on the following:

https://github.com/awslabs/aws-sdk-arduino

See section "Table used by SparkGetItemSample and GalileoSample:"





