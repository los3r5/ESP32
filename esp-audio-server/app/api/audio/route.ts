import type { NextApiRequest, NextApiResponse } from 'next';
import { getAudioServer } from "@/app/server"

type AudioResponse = {
  status: string;
  data?: any;
  error?: string;
  recordingUrl?: string;
}

export default async function handler(
  req: NextApiRequest,
  res: NextApiResponse<AudioResponse>
) {
  // Get server instance
  const audioServer = getAudioServer();

  // Handle different endpoints
  switch (req.query.action) {
    case 'save':
      try {
        const recordingUrl = await audioServer.saveRecording();
        res.status(200).json({ 
          status: 'success', 
          recordingUrl: recordingUrl || undefined 
        });
      } catch (error) {
        res.status(500).json({ 
          status: 'error', 
          error: 'Failed to save recording' 
        });
      }
      break;
      
    case 'data':
      try {
        const audioData = audioServer.getRecentAudioData();
        res.status(200).json({ 
          status: 'success', 
          data: audioData 
        });
      } catch (error) {
        res.status(500).json({ 
          status: 'error', 
          error: 'Failed to get audio data' 
        });
      }
      break;
      
    default:
      res.status(400).json({ 
        status: 'error', 
        error: 'Invalid action' 
      });
  }
}