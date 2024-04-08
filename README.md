# DANGER RANGER - Threat Assesser

### Created by: Matthew Denton (Lead Programmer), Garrett Bradham (Lead Designer), Joshua Potts (Head of Marketing), and Dani (Russian Spy)

# HACK BETA '24 1st Place🥇

Under the name "Here For Free Food," our group placed 1st at the University of Mississippi's Hack BETA 2024.

## What It Does

Our Application collects data about ambient temperature, humidity, and barometric pressure, measuring the current environment's threat level as defined by the user. It then tabulates this data, informing the user of the current threat level of their enviroment alongside storing recent data. The user can then set a custom range for each reading depending on their environment and our application will assess the threat level based on the user's ranges. Additionally, this data is sent through encrypted transmissions that allows a user to have real-time information on their environment and alerts to threats while also providing saftey and comfort knowing no one else may have access to this sensitive data.

## How We Built It

For this project we used our smarts, creativity, and overall frustration to allow the provided esp8266 modules to communicate with each other, preform threat assessment, and provide essential data to the user. This essential data is then tracked and uploaded to a self-hosted web server on the esp8266-OLED module. Afterwards, threat levels are caculated and then updated accordingly to the user's device as well as to the oled screen. We had four modules in total each with it's own function. The oled module being the 'brain' of our system, served as a WiFi access point that allowed users to connect and utilize the html page within to view diagnostic information about the other devices. Our three other modules, one for temperature, one for humidity, and one button, utilized the ESPNOW protocol to send and recieve data from one another over encrypted channels.

## Challenges

Coming into this project, our group had very little experience working with ARDUINO boards. We also lacked coding experience using languages such as Javascript and C++. Additionally, the provided esp8266 boards left little leeway for inoptimal code making it quite the challenge when working with so many devices, constantly running the risk of memory leakage. Even so, our group worked hard with the time we had to find optimal solutions to the problems we faced.

## Takeaways

Although this was a challenging hackathon, we were able to take our previous knowledge and apply it in a new field in order to produce working software, and our placement proves that we were able to swiftly and efficiently tackle a real-world problem.
