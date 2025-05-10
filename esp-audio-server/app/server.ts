import dgram from 'dgram';
import fs from 'fs';
import path from 'path';
import WavEncoder from 'wav-encoder';
import bufferToArrayBuffer from 'buffer-to-arraybuffer';

// Audio settings
const CHANNELS = 1;
const SAMPLE_RATE = 16000;
const SAMPLE_WIDTH = 2; // 16-bit

// UDP Server configuration
const UDP_PORT = 3333;

class AudioServer {
  private server: dgram.Socket;
  private recording: Buffer[] = [];
  private isRecording: boolean = false;
  private recordingStart: Date | null = null;
  private audioChunks: Int16Array[] = [];
  
  // Optional: Connection status callback for frontend
  private statusCallback: ((status: string) => void) | null = null;

  constructor() {
    this.server = dgram.createSocket('udp4');
    this.setupServer();
  }

  private setupServer() {
    this.server.on('error', (err) => {
      console.error(`UDP server error: ${err.message}`);
      this.server.close();
    });

    this.server.on('message', (msg, rinfo) => {
      if (!this.isRecording) {
        this.isRecording = true;
        this.recordingStart = new Date();
        console.log(`Started recording from ${rinfo.address}:${rinfo.port}`);
        
        if (this.statusCallback) {
          this.statusCallback(`Connected to ESP32 at ${rinfo.address}:${rinfo.port}`);
        }
      }

      // Add the buffer to our recording
      this.recording.push(msg);

      // Convert buffer to Int16Array for visualizing or processing
      const audioChunk = new Int16Array(bufferToArrayBuffer(msg));
      this.audioChunks.push(audioChunk);

      // If we have too many chunks, remove old ones
      if (this.audioChunks.length > 100) {
        this.audioChunks.shift();
      }

      // Save recording to file every 10 seconds
      const now = new Date();
      if (this.recordingStart && 
          (now.getTime() - this.recordingStart.getTime() > 10000) && 
          this.recording.length > 0) {
        this.saveRecording();
      }
    });

    this.server.on('listening', () => {
      const address = this.server.address();
      console.log(`UDP Server listening on ${address.address}:${address.port}`);
      
      if (this.statusCallback) {
        this.statusCallback(`UDP Server running on port ${address.port}`);
      }
    });

    this.server.bind(UDP_PORT);
  }

  public async saveRecording() {
    if (this.recording.length === 0) return;

    // Concatenate all buffers
    const completeBuffer = Buffer.concat(this.recording);
    
    // Convert Buffer to ArrayBuffer
    const arrayBuffer = bufferToArrayBuffer(completeBuffer);
    
    // Convert ArrayBuffer to Int16Array for WAV encoding
    const int16Array = new Int16Array(arrayBuffer);
    
    // Format for WAV Encoder
    const audioData = {
      sampleRate: SAMPLE_RATE,
      channelData: [Array.from(int16Array)]
    };
    
    try {
      // Encode as WAV
      const wavBuffer = await WavEncoder.encode(audioData);
      
      // Create timestamps directory if it doesn't exist
      const dir = path.join(process.cwd(), 'public', 'recordings');
      if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
      }
      
      // Generate filename with timestamp
      const timestamp = new Date().toISOString().replace(/[:.]/g, '-');
      const filename = path.join(dir, `audio_${timestamp}.wav`);
      
      // Write to file
      fs.writeFileSync(filename, Buffer.from(wavBuffer));
      console.log(`Saved recording to ${filename}`);
      
      // Reset recording
      this.recording = [];
      this.recordingStart = new Date();
      
      return `/recordings/audio_${timestamp}.wav`;
    } catch (error) {
      console.error('Error saving WAV file:', error);
      return null;
    }
  }

  public getRecentAudioData(): number[] {
    // Combine recent chunks and return for visualization
    if (this.audioChunks.length === 0) return [];
    
    // Combine the last few chunks into a single array
    const combined = new Int16Array(this.audioChunks.reduce((acc, chunk) => acc + chunk.length, 0));
    let offset = 0;
    
    for (const chunk of this.audioChunks) {
      combined.set(chunk, offset);
      offset += chunk.length;
    }
    
    // Downsample to a reasonable size for visualization
    const downsampleFactor = Math.max(1, Math.floor(combined.length / 1000));
    const result: number[] = [];
    
    for (let i = 0; i < combined.length; i += downsampleFactor) {
      result.push(combined[i]);
    }
    
    return result;
  }

  public setStatusCallback(callback: (status: string) => void) {
    this.statusCallback = callback;
  }

  public close() {
    if (this.recording.length > 0) {
      this.saveRecording();
    }
    this.server.close();
  }
}

// Singleton instance
let audioServer: AudioServer | null = null;

export function getAudioServer(): AudioServer {
  if (!audioServer) {
    audioServer = new AudioServer();
  }
  return audioServer;
}