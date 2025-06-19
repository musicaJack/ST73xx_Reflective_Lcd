import os

def process_text_file(input_file, output_file):
    # 读取输入文件（保留所有行，包括空行）
    with open(input_file, 'r', encoding='utf-8') as f:
        lines = [line.rstrip('\n') for line in f]  # 只去除行尾换行符
    
    # 处理每一行
    processed_lines = []
    for line in lines:
        # 空行处理
        if not line:
            processed_lines.append('""')
            continue
        
        # 转义特殊字符（双引号和反斜杠）
        escaped_line = line.replace('\\', '\\\\').replace('"', '\\"')
        processed_lines.append(f'"{escaped_line}"')
    
    # 生成输出内容
    output_content = """#pragma once
#include <vector>
#include <string>

// 定义全局变量，存储小说内容
const std::vector<std::string> the_little_prince_content = {{
    {content_lines}
}};
""".format(content_lines=",\n    ".join(processed_lines))
    
    # 写入输出文件
    with open(output_file, 'w', encoding='utf-8') as f:
        f.write(output_content)

if __name__ == "__main__":
    # 输入输出路径配置
    input_path = os.path.join(os.getcwd(), "the_little_prince.txt")
    output_path = os.path.join(os.getcwd(), "the_little_prince_content.hpp")
    
    # 检查输入文件是否存在
    if not os.path.exists(input_path):
        print(f"错误：输入文件不存在 - {input_path}")
    else:
        process_text_file(input_path, output_path)
        print(f"成功生成完整输出文件: {output_path}")
        print(f"共处理 {len(open(input_path, 'r', encoding='utf-8').readlines())} 行文本")