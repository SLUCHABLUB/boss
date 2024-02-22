'''
This module runs a web server and prints input from http-requests to boss's LED display using scrolling-text-example.
Two threads are used, one which listens to http-requests and adds incoming messages to a queue, and one which handles what's being shown on the LED using the queue.
If there are messages in the queue they are printed on the LED, otherwise some demos are shown instead.
The demos are run in a subprocess which the thread kills if there arrive any new messages.
'''
from asyncio import subprocess
from time import sleep, time
from urllib import response
from bottle import run, request, post, response
import subprocess
import threading
import random
import os
import glob

pathToFiles = '/home/pi/rpi-rgb-led-matrix/examples-api-use/'
baseArguments = '--led-rows=32 --led-cols=64 --led-chain=3 --led-gpio-mapping=adafruit-hat-pwm --led-slowdown-gpio=15 --led-show-refresh'

videos_folder = '/home/pi/boss_videos'
video_extentions = ["mp4", "gif", "webm"]

#A dictionary used to convert the URL arguments to valid arguments for the underlying program
argumentConvertDictionary = {
    'speed':'-s',
    #'loop':'-l',    #currently locked to 1
    #'font':'-f',    #currently locked to a specific font
    'brightness':'-b',
    'xorigin':'-x',
    'yorigin':'-y',
    'spacing':'-S',
    'color':'-C',
    'background':'-B',
    'outline':'-O'
}

messageQueue = []
queueSize = 10

def runTextOnLED():
    loopAndFont = '-l 1 -f /home/pi/rpi-rgb-led-matrix/fonts/spleen-16x32.bdf'
    global demoProcess      #making the subprocess global so another thread can kill it outside of this function
    demoProcess = runDemosOnLoop()
    demoStartTime = time()
    while(True):
        if messageQueue:
            demoProcess.terminate()
            while messageQueue:
                userInput = messageQueue.pop(0)
                print(userInput)
                userInputFixed = userInput[1].replace("\\", "\\\\")
                userInputFixed = userInputFixed.replace("\"", "\\\"")
                userInputFixed = userInputFixed.replace("\"", "\\\"")
                # os.system(f'sudo {pathToFiles}scrolling-text-example {baseArguments} {loopAndFont} {userInputFixed}')    #must add input validation somewhere(Stringify/URLEncode/escaping?) currently user input is run as sudo, DANGEROUS
                p = subprocess.Popen([f'{pathToFiles}scrolling-text-example'] +baseArguments.split(' ') +loopAndFont.split(' ') +userInput[0].split(' ') +[f'{userInputFixed}'])
                pid = p.pid
                os.waitpid(pid, 0)
        if demoProcess.poll() == None and time() - demoStartTime >= 30:
            demoProcess.terminate()
        if demoProcess.poll() != None:
            demoStartTime = time()
            demoProcess = runDemosOnLoop()
        sleep(1)

def runDemosOnLoop():
    choice = random.randint(1, 16)
    if choice <= 3:
        algorithmsAndDelays = [('insertion', 10), ('cocktail', 100)]
        algo, delay = random.choice(algorithmsAndDelays)
        cmd = f'{pathToFiles}sort -s {algo} {baseArguments} -C 252,128,161 -d {delay}'.split(' ')
    elif choice <= 6:
        cmd = f'{pathToFiles}mandelbrot -d 160 -z 1.11 -i 250 -t 160 {baseArguments}'.split(' ')
    elif choice <= 10:
        videos = []
        for extension in video_extentions:
            videos.extend(glob.glob(f'{videos_folder}/*.{extension}'))
        video = random.choice(videos)
        cmd = f'/home/pi/rpi-rgb-led-matrix/utils/video-viewer {baseArguments} -F {video}'.split(' ')
    elif choice <= 13:
        cmd = f'{pathToFiles}choochoo {baseArguments}'.split(' ')
    else:
        demoNumbers = [4, 7, 7, 7, 9, 10]	#"game of life"-demo is weighted
        randomDemo = random.choice(demoNumbers)
        cmd = f'{pathToFiles}demo -D {randomDemo} {baseArguments}'.split(' ')
    return subprocess.Popen(cmd, stdin=None, stdout=None)

@post('/sendText')
def sendText():
    if len(messageQueue) > queueSize:
        response.status = 400
        return 'Too many messages'
    message = request.query.decode().message      #TODO: check if message needs to be at the end of the system call, otherwise modify the code to treat message as a normal argument
    argumentString = ''
    for k,v in request.query.allitems():
        if not k == 'message':
            argumentString += f'{argumentConvertDictionary[k]} {v} '
    messageQueue.append((argumentString, message[:250]))

@post('/json')
def jsonText():
    if len(messageQueue) > queueSize:
        response.status = 400
        return 'Too many messages'
    message = request.json['message'] #TODO: check if message needs to be at the end of the system call, otherwise modify the code to treat message as a normal argument
    argumentString = ''
    for k,v in request.json.items():
        if not k == 'message':
            argumentString += f'{argumentConvertDictionary[k]} {v} '
    messageQueue.append(f'{argumentString} \"{message}\"')

def runServer():
    run(host='0.0.0.0', port=8080)

if __name__ == '__main__':
    try:
        LEDThread = threading.Thread(target=runTextOnLED, args=())
        LEDThread.start()
        run(host='0.0.0.0', port=8080)
    except KeyboardInterrupt:
        LEDThread.kill()
        demoProcess.kill()
        print("end reached")
