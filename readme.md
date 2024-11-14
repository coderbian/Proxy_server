Tiến độ:
+ Đã mở được nhiều tab / website
+ Đã chặn được những website cần chặn (config\blacklist.txt)

Chưa làm được:
+ Các Active Threads của những tabs đã tắt vẫn mất rất nhiều thời gian để kết thúc (ước tính tầm 30s cũng tạm chấp nhận được).
+ Chưa lưu được dữ liệu về trên máy chủ để cấp phát cho máy con.
+ ...

Một số chú ý về các files:
+ .gitignore: Để chặn các folders - files không muốn đẩy lên github.
+ makefile:
    - make    : dịch code 
    - make run: dịch code và chạy
    - Khuyến khích dùng 'start make run' để xem được rõ ràng hơn.
    - Cài make bằng msys2 (update sau) 