## Ghi chú ổn định mạng Mesh

- **Giữ nguyên kênh WiFi**: tránh để router tự động thay đổi kênh; nên cố định ví dụ kênh 6.
- **Giữ nguyên SSID & password**: nếu có nhiều mạng mesh song song, đặt SSID riêng để tránh nhiễu.
- **Giới hạn số lần scan**: dùng `mesh.setContainsRoot(true)` để cố định root, tránh node liên tục tìm kiếm và chuyển giữa các node.
- **Quản lý Root node (Gateway)**:
  - Chỉ định một thiết bị mạnh (ví dụ ESP32) làm root cố định và cấp nguồn ổn định; nếu root không ổn định cả mạng sẽ nhảy loạn.
  - Root không được sleep: tắt chế độ tiết kiệm điện để tránh node con mất kết nối.