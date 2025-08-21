# JKSV Google Drive Instructions (2025)

**Update 2025 / JKSV rewrite:**
JKSV's rewrite returns to using UTF-8 titles for Google Drive like the original JKSV did in the beginning. This change was made by a less skilled developer than myself. I apologize for this. I should have scrutinized this decision, _and the WebDav code in general_, more before merging. I mistakenly assumed using "path safe" titles would only apply to WebDav and not be applied to both. I apologize for any issues this has caused.

**Note:** Throughout this guide, clickable items in screenshots will be highlighted in **red**. I did the best I could with Markdown's limitations.

---

### 1. Create a new project
1. Go to [Google Cloud Console](https://console.cloud.google.com/welcome/new) and click **Select a project** (top-left corner).
<details>
<summary>Screenshot</summary>
<p align="center">
<img src="https://i.imgur.com/w4HjU9y.png">
</p>
</details>
<br>

2. Click **New Project** (top-right). Name it `JKSV`. It's fine to leave **Organization** blank.

<details>
<summary>Screenshots</summary>
<p align="center">
<img src="https://i.imgur.com/y3R22R4.png">
<img src="https://i.imgur.com/bPXMxUD.png">
</p>
</details>
<br>

3. Wait for the project to be created, then click **Select Project**.

<details>
<summary>Screenshot</summary>
<p align="center">
<img src="https://i.imgur.com/NrhZoC3.png">
</p>
</details>
<br>

---

### 2. Enable the Google Drive API
1. Click the **Navigation menu** (top-left) → **APIs & Services** → **Enabled APIs & Services**.
<details>
<summary>Screenshots</summary>
<p align="center">
<img src="https://i.imgur.com/ghEEXO6.png">
<img src="https://i.imgur.com/pzSlQ3g.png">
</p>
</details>
<br>

2. Click **+ Enable APIs and services**, scroll to **Google Workspace**, select **Google Drive API**, and click **Enable**.
<details>
<summary>Screenshots</summary>
<p align="center">
<img src="https://i.imgur.com/92rMLyv.png">
<img src="https://i.imgur.com/1bDWDQg.png">
<img src="https://i.imgur.com/DlYQwJo.png">
</p>
</details>
<br>

---

### 3. Create OAuth credentials
1. Go to **Credentials** → **+ Create Credentials** → **OAuth client ID**.
<details>
<summary>Screenshot</summary>
<p align="center">
<img src="https://i.imgur.com/FKneOkM.png">
</p>
</details>
<br>

2. Click **Configure consent screen**.
 - Set the **Application Type** to **TVs and limited input devices**.
 - **Download the JSON file containing your client info! Google no longer allows you to download this again after this point!**
<details>
<summary>Screenshots</summary>
<p align="center">
<img src="https://i.imgur.com/u2MrN8y.png">
<img src="https://i.imgur.com/sRI8ykE.png">
<img src="https://i.imgur.com/GmqyNJ9.png">
</p>
</details>
<br>

3. Rename the downloaded file to `client_secret.json` and store it somewhere safe.

---

### 4. Add yourself as a test user
1. Go to **Navigation menu** → **APIs & Services** → **OAuth consent screen** → **Audience**.
2. Click **+ Add users** and add your email.
<details>
<summary>Screenshots</summary>
<p align="center">
<img src="https://i.imgur.com/Tn3zmO1.png">
<img src="https://i.imgur.com/OhyztMJ.png">
</p>
</details>
<br>

---

### 5. Copy the JSON to your Switch
- Move the JSON to: `sdmc:/config/JKSV/client_secret.json`.
- The next time JKSV starts with an internet connection, it will guide you to complete the login process.

---

### **6. (OPTIONAL BONUS STEP!)**
- Make a secure backup of `client_secret.json` from your Switch's SD card **after logging in with JKSV**.
- This is recommended if you plan to share saves between multiple Switches due to Google’s OAuth scope policies.
