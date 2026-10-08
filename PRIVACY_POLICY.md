# Privacy Policy for Apex Overdrive

**Last Updated:** October 8, 2026  
**Effective Date:** October 8, 2026  
**Application Name:** Apex Overdrive  
**Developer / Publisher:** Jacky Phuti  
**Contact Email:** jackyphuti@users.noreply.github.com  
**Repository:** https://github.com/jackyphuti/Game-Optimizer  

---

## 1. Introduction
This Privacy Policy describes how **Apex Overdrive** ("we", "our", or "the Application") handles information when you install, run, or interact with the software on Windows or Linux operating systems. 

We are committed to user privacy and software transparency. **Apex Overdrive operates strictly offline on your local device. We do not collect, transmit, store, sell, or share any personal data.**

---

## 2. Information Handled by the Application
To deliver real-time performance optimization, automotive cluster telemetry, and hardware benchmark scores, Apex Overdrive inspects specific local system hardware parameters:

### A. Local Hardware Telemetry (Processed in Memory Only)
- **Central Processing Unit (CPU):** Model name, physical core and thread count, base and boost clock frequency, and real-time percentage utilization.
- **Graphics Processing Unit (GPU):** Model name, dedicated video memory (VRAM), display resolution, driver version, and utilization percentage.
- **Random Access Memory (RAM):** Total installed physical memory, available capacity, and memory speed.
- **Storage Drives:** Drive model names, interface type (e.g., NVMe PCIe SSD), and capacity.
- **Operating System Version:** OS release and architecture for power plan and scheduling quantum compatibility.

> **Important:** All hardware information and metrics are queried locally using standard operating system APIs and the open-source `systeminformation` library. This data is processed strictly in volatile memory (RAM) while the application is active and is **never** sent to any external server or third party.

### B. Personal Data
Apex Overdrive **does not** collect:
- Names, addresses, telephone numbers, or email addresses.
- Passwords, credentials, or authentication tokens.
- User files, documents, browsing history, or personal media.
- Device unique identifiers, advertising IDs, or telemetry cookies.

---

## 3. Permissions & System-Level Changes

To execute its performance optimization routines, Apex Overdrive interacts with Windows system features. All modifications are transparent, non-destructive, and reversible:

1. **Power Plan Management:** Switches the active Windows power profile between "Balanced" and "High / Ultimate Performance" via standard Windows APIs (`powercfg`).
2. **Processor Scheduling & Core Parking:** Bypasses low-power core parking states while gaming mode is active to prevent stuttering.
3. **Multimedia Timer Resolution:** Temporarily lowers scheduling quantum from 15.6ms to 0.5ms to minimize input latency.
4. **Memory Working Set Flush:** Calls Windows API memory management functions (`EmptyWorkingSet` and GC) to release idle standby memory.
5. **Process Priority:** Elevates game process priorities (`HIGH_PRIORITY_CLASS`) and throttles background launcher processes.

**Safety Guarantee:** Apex Overdrive includes an automatic rollback mechanism (`OptimizationGuard` and Rollback Journal). All settings are reverted to defaults when the game exits or when clicking "Restore Defaults".

---

## 4. Third-Party Services & Network Access
Apex Overdrive does not integrate any third-party analytics (e.g., Google Analytics, Mixpanel), advertisement networks, or user-tracking SDKs. The application operates completely self-contained without internet connectivity requirements.

---

## 5. Data Security
Because no personal information or user telemetry is collected, stored, or transmitted over the network, your data is never subject to cloud data breaches or external vulnerabilities through Apex Overdrive.

---

## 6. Children's Privacy
Apex Overdrive does not collect information from anyone, including children under the age of 13. The application is completely safe and suitable for all audiences.

---

## 7. Changes to This Privacy Policy
We may periodically update this Privacy Policy to reflect software updates or regulatory changes. Any updates will be published with an updated "Last Updated" date at the repository URL:  
https://github.com/jackyphuti/Game-Optimizer/blob/main/PRIVACY_POLICY.md

---

## 8. Contact Information
If you have any questions or feedback regarding this Privacy Policy or the security practices of Apex Overdrive, please contact:

- **Publisher:** Jacky Phuti
- **Email:** jackyphuti@users.noreply.github.com
- **Project URL:** https://github.com/jackyphuti/Game-Optimizer/issues
