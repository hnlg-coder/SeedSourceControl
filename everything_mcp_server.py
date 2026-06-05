#!/usr/bin/env python3
"""
Everything Search MCP Server
Provides file search capabilities using Everything SDK
"""

import json
import subprocess
import sys
from typing import List, Dict, Any, Optional

# Try to import Everything SDK if available
try:
    from everything import Everything, EVERYTHING_SORT_NAME_ASCENDING, EVERYTHING_REQUEST_FILE_NAME, EVERYTHING_REQUEST_PATH
    HAS_SDK = True
except ImportError:
    HAS_SDK = False

def search_everything(query: str, max_results: int = 50, 
                       file_type: Optional[str] = None,
                       min_size: Optional[int] = None,
                       max_size: Optional[int] = None,
                       modified_after: Optional[str] = None,
                       modified_before: Optional[str] = None) -> List[Dict[str, Any]]:
    """
    Search using Everything SDK or fallback to es.exe
    
    Args:
        query: Search query string
        max_results: Maximum number of results
        file_type: Filter by file extension (e.g., ".txt", ".pdf")
        min_size: Minimum file size in bytes
        max_size: Maximum file size in bytes
        modified_after: Modified after date (YYYY-MM-DD format)
        modified_before: Modified before date (YYYY-MM-DD format)
    """
    results = []
    
    if HAS_SDK:
        # Use SDK
        everything = Everything()
        everything.set_search(query)
        everything.set_request_flags(EVERYTHING_REQUEST_FILE_NAME | EVERYTHING_REQUEST_PATH)
        everything.set_sort(EVERYTHING_SORT_NAME_ASCENDING)
        
        if everything.execute():
            for i in range(min(everything.get_num_results(), max_results)):
                name = everything.get_result_file_name(i)
                path = everything.get_result_path(i)
                
                result = {
                    "name": name,
                    "path": path,
                    "type": "file"
                }
                
                # Apply filters
                if file_type and not name.lower().endswith(file_type.lower()):
                    continue
                
                results.append(result)
    else:
        # Fallback to es.exe
        es_path = r"C:\Program Files\Everything\es.exe"
        
        # Build command
        cmd = [es_path, "-p", query]
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            
            if result.returncode == 0:
                for line in result.stdout.strip().split('\n'):
                    if line and line.strip():
                        path = line.strip()
                        name = path.split('\\')[-1] if '\\' in path else path.split('/')[-1]
                        
                        result_item = {
                            "name": name,
                            "path": path,
                            "type": "file"
                        }
                        
                        # Apply filters
                        if file_type and not name.lower().endswith(file_type.lower()):
                            continue
                        
                        results.append(result_item)
                        
                        if len(results) >= max_results:
                            break
        except Exception as e:
            print(f"Error running es.exe: {e}", file=sys.stderr)
    
    return results

def format_results(results: List[Dict[str, Any]], format_type: str = "json") -> str:
    """Format search results"""
    if format_type == "json":
        return json.dumps(results, indent=2, ensure_ascii=False)
    elif format_type == "markdown":
        output = "## Search Results\n\n"
        output += "| Name | Path |\n"
        output += "|------|-------|\n"
        for r in results:
            output += f"| {r['name']} | {r['path']} |\n"
        return output
    else:
        return "\n".join([f"{r['name']}: {r['path']}" for r in results])

def main():
    """MCP Server main entry"""
    # Read input from stdin
    try:
        line = sys.stdin.readline()
        request = json.loads(line)
    except Exception as e:
        print(f"Error reading request: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Parse request
    method = request.get("method", "")
    params = request.get("params", {})
    
    if method == "tools/call":
        tool_name = params.get("name", "")
        arguments = params.get("arguments", {})
        
        if tool_name == "everything_search":
            query = arguments.get("query", "")
            max_results = arguments.get("max_results", 50)
            file_type = arguments.get("file_type", None)
            min_size = arguments.get("min_size", None)
            max_size = arguments.get("max_size", None)
            modified_after = arguments.get("modified_after", None)
            modified_before = arguments.get("modified_before", None)
            format_type = arguments.get("format", "json")
            
            results = search_everything(
                query=query,
                max_results=max_results,
                file_type=file_type,
                min_size=min_size,
                max_size=max_size,
                modified_after=modified_after,
                modified_before=modified_before
            )
            
            formatted = format_results(results, format_type)
            
            response = {
                "jsonrpc": "2.0",
                "result": {
                    "content": [
                        {
                            "type": "text",
                            "text": formatted
                        }
                    ]
                }
            }
        else:
            response = {
                "jsonrpc": "2.0",
                "error": {
                    "code": -32601,
                    "message": f"Unknown tool: {tool_name}"
                }
            }
    else:
        response = {
            "jsonrpc": "2.0",
            "error": {
                "code": -32601,
                "message": f"Unknown method: {method}"
            }
        }
    
    # Write response
    print(json.dumps(response), flush=True)

if __name__ == "__main__":
    main()
