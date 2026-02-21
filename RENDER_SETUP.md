# Render.com Deployment Guide - Vantage Game Server

## Prerequisites
- GitHub account (free)
- Render.com account (free, NO credit card needed)

## Step 1: Create GitHub Repository

1. Go to https://github.com/new
2. Create new repository:
   - **Name:** vantage-game-server
   - **Visibility:** Public (or Private if you prefer)
   - **Don't** initialize with README (we have files already)
3. Click **Create repository**

## Step 2: Push Your Code to GitHub

Open Git Bash in your game folder and run:

```bash
# Initialize git (if not already done)
git init

# Add all files
git add .

# Commit
git commit -m "Initial commit - Vantage game server"

# Add your GitHub repo as remote (replace YOUR_USERNAME)
git remote add origin https://github.com/YOUR_USERNAME/vantage-game-server.git

# Push to GitHub
git branch -M main
git push -u origin main
```

**Note:** Replace `YOUR_USERNAME` with your actual GitHub username!

## Step 3: Sign Up for Render.com

1. Go to https://render.com/
2. Click **Get Started**
3. Sign up with GitHub (easiest - one click)
4. Authorize Render to access your repositories

## Step 4: Deploy Your Server

1. In Render dashboard, click **New +**
2. Select **Web Service**
3. Connect your repository:
   - Find **vantage-game-server**
   - Click **Connect**
4. Configure:
   - **Name:** vantage-game-server
   - **Environment:** Docker
   - **Plan:** Free
   - **Advanced:**
     - Add environment variable:
       - Key: `PORT`
       - Value: `7777`
5. Click **Create Web Service**

## Step 5: Wait for Deployment

- Render will build your Docker image (takes 2-5 minutes)
- Watch the logs in real-time
- When you see "Server started on port 7777" - it's ready! ✅

## Step 6: Get Your Server URL

After deployment:
- Your server URL will be: `https://vantage-game-server.onrender.com`
- **But for UDP, you need the IP address**

To get the IP:
```bash
# On Windows (Command Prompt or PowerShell)
nslookup vantage-game-server.onrender.com

# Or use online tool:
# https://www.nslookup.io/
```

Copy the IP address (e.g., `216.24.57.1`)

## Step 7: Configure Your Game Client

In your game client code, connect to:
- **Server IP:** (the IP from nslookup)
- **Port:** 7777

Example:
```cpp
const char* SERVER_IP = "216.24.57.1";  // Your Render server IP
const int SERVER_PORT = 7777;
```

## Important Notes

### Free Tier Limitations:
- ✅ 750 hours/month (enough for 24/7 if only 1 server)
- ✅ Unlimited players (within reason)
- ⚠️ Server sleeps after 15 minutes of inactivity
- ⚠️ Takes ~30 seconds to wake up when player connects

### Server Sleep Behavior:
- Server automatically sleeps after 15 min with no connections
- First player to connect will wait ~30 seconds for wake-up
- After wake-up, instant for other players
- To prevent sleep: Keep 1 player connected, or upgrade to paid ($7/month)

### Updating Your Server:
```bash
# Make changes to game_server.cpp
git add .
git commit -m "Update server code"
git push

# Render auto-deploys on push! (takes 2-3 minutes)
```

## Troubleshooting

### Build fails:
- Check Render logs for errors
- Make sure all files are committed to GitHub
- Verify Dockerfile syntax

### Can't connect:
- Make sure you're using the IP address, not the URL
- Verify port 7777
- Check server logs in Render dashboard

### Server keeps sleeping:
- This is normal on free tier
- First connection takes 30 seconds to wake
- Consider keeping a "dummy" client connected
- Or upgrade to paid tier ($7/month for always-on)

## Monitoring

In Render dashboard:
- **Logs:** Real-time server output
- **Metrics:** CPU, memory, bandwidth usage
- **Events:** Deployments, crashes, restarts

## Cost

**Free Tier:**
- 750 hours/month
- Sleeps after 15 min inactivity
- **Cost: $0/month**

**Paid Tier (optional):**
- Always-on (no sleep)
- More resources
- **Cost: $7/month**

For testing and small games, free tier is perfect!

## Your Server Info

Once deployed, save these:
- **GitHub Repo:** https://github.com/YOUR_USERNAME/vantage-game-server
- **Render URL:** https://vantage-game-server.onrender.com
- **Server IP:** (get from nslookup)
- **Port:** 7777

Share the IP and port with your friends to play!

## Next Steps

1. ✅ Deploy server to Render
2. ⏳ Modify game client to connect to server
3. ⏳ Test with friends
4. ⏳ Add Supabase lobby (optional)

Ready to modify your game client? Let me know!
