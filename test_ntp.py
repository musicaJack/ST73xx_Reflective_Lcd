#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
NTP服务器连通性测试程序
用于验证NTP服务器是否可达和响应
"""

import socket
import struct
import time
import sys

def create_ntp_packet():
    """创建NTP请求包"""
    # NTP包格式：48字节
    # LI=0, VN=4, Mode=3 (客户端模式)
    ntp_packet = bytearray(48)
    ntp_packet[0] = 0x1b  # 00011011: LI=0, VN=4, Mode=3
    return ntp_packet

def parse_ntp_response(data):
    """解析NTP响应包"""
    if len(data) < 48:
        return None
    
    # 提取传输时间戳（字节40-47）
    timestamp_bytes = data[40:48]
    timestamp = struct.unpack('!Q', timestamp_bytes)[0]
    
    # NTP时间戳是从1900年1月1日开始的秒数
    # Unix时间戳是从1970年1月1日开始的秒数
    # 差值：70年 = 2208988800秒
    ntp_epoch_offset = 2208988800
    
    # 转换为秒和小数部分
    seconds = (timestamp >> 32) & 0xFFFFFFFF
    fraction = timestamp & 0xFFFFFFFF
    
    # 转换为Unix时间戳
    unix_timestamp = seconds - ntp_epoch_offset
    
    return unix_timestamp

def test_ntp_server(server, port=123, timeout=10):
    """测试NTP服务器"""
    print(f"\n{'='*50}")
    print(f"测试NTP服务器: {server}:{port}")
    print(f"{'='*50}")
    
    try:
        # 解析服务器地址
        print(f"正在解析地址 {server}...")
        try:
            # 检查是否是IP地址
            socket.inet_aton(server)
            server_ip = server
            print(f"✅ 直接使用IP地址: {server_ip}")
        except socket.error:
            # 不是IP地址，需要DNS解析
            print(f"正在进行DNS解析...")
            server_ip = socket.gethostbyname(server)
            print(f"✅ DNS解析成功: {server} -> {server_ip}")
        
        # 创建UDP套接字
        print(f"创建UDP套接字...")
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.settimeout(timeout)
        
        # 创建NTP请求包
        print(f"创建NTP请求包...")
        ntp_packet = create_ntp_packet()
        
        # 发送请求
        print(f"发送NTP请求到 {server_ip}:{port}...")
        start_time = time.time()
        sock.sendto(ntp_packet, (server_ip, port))
        
        # 接收响应
        print(f"等待NTP响应（{timeout}秒超时）...")
        data, addr = sock.recvfrom(1024)
        end_time = time.time()
        
        # 关闭套接字
        sock.close()
        
        # 计算往返时间
        rtt = (end_time - start_time) * 1000  # 毫秒
        
        print(f"✅ 收到NTP响应:")
        print(f"   - 响应地址: {addr[0]}:{addr[1]}")
        print(f"   - 响应大小: {len(data)} 字节")
        print(f"   - 往返时间: {rtt:.2f} ms")
        
        # 解析NTP时间
        if len(data) >= 48:
            unix_timestamp = parse_ntp_response(data)
            if unix_timestamp:
                # 转换为北京时间 (UTC+8)
                beijing_time = time.localtime(unix_timestamp + 8*3600)
                time_str = time.strftime("%Y-%m-%d %H:%M:%S", beijing_time)
                print(f"   - NTP时间: {time_str} (北京时间)")
                
                # 与本地时间比较
                local_time = time.time()
                time_diff = abs(unix_timestamp - local_time)
                print(f"   - 时间差: {time_diff:.2f} 秒")
                
                if time_diff < 5:
                    print(f"   ✅ 时间同步良好")
                elif time_diff < 60:
                    print(f"   ⚠️ 时间差较大")
                else:
                    print(f"   ❌ 时间差过大")
            else:
                print(f"   ❌ 无法解析NTP时间")
        else:
            print(f"   ❌ NTP响应包太短")
            
        return True
        
    except socket.timeout:
        print(f"❌ 连接超时 ({timeout}秒)")
        return False
    except socket.gaierror as e:
        print(f"❌ DNS解析失败: {e}")
        return False
    except socket.error as e:
        print(f"❌ 网络错误: {e}")
        return False
    except Exception as e:
        print(f"❌ 其他错误: {e}")
        return False

def main():
    """主函数"""
    print("NTP服务器连通性测试程序")
    print("=" * 50)
    
    # 常用的NTP服务器列表
    ntp_servers = [
        # 中国NTP服务器
        "cn.pool.ntp.org",
        "119.28.206.193",  # 腾讯云NTP
        "120.25.115.20",   # 阿里云NTP
        "182.92.12.11",    # 网易NTP
        
        # 国际NTP服务器
        "pool.ntp.org",
        "time.nist.gov",
        "time.google.com",
        
        # 其他备用服务器
        "ntp1.aliyun.com",
        "ntp2.aliyun.com",
        "time1.cloud.tencent.com",
    ]
    
    successful_servers = []
    failed_servers = []
    
    for server in ntp_servers:
        try:
            if test_ntp_server(server, timeout=5):
                successful_servers.append(server)
            else:
                failed_servers.append(server)
        except KeyboardInterrupt:
            print(f"\n用户中断测试")
            break
        except Exception as e:
            print(f"测试 {server} 时发生错误: {e}")
            failed_servers.append(server)
        
        time.sleep(1)  # 避免请求过于频繁
    
    # 显示测试结果摘要
    print(f"\n{'='*50}")
    print(f"测试结果摘要")
    print(f"{'='*50}")
    print(f"成功的服务器 ({len(successful_servers)}):")
    for server in successful_servers:
        print(f"  ✅ {server}")
    
    print(f"\n失败的服务器 ({len(failed_servers)}):")
    for server in failed_servers:
        print(f"  ❌ {server}")
    
    if successful_servers:
        print(f"\n推荐使用的NTP服务器:")
        print(f"  1. {successful_servers[0]} (首选)")
        if len(successful_servers) > 1:
            print(f"  2. {successful_servers[1]} (备选)")

if __name__ == "__main__":
    main() 