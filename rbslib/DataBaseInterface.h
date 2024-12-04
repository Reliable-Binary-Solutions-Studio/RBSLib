#pragma once
#include <vector>
#include <string>
namespace RbsLib::DataBase
{
/*
* 数据库接口一般应不可拷贝，对于多处需要访问时建议使用智能指针或者引用
*/
	class DataBaseInterface
	{
	public:
		virtual void Connect(const std::string& connection_string) = 0;
		virtual void Disconnect() = 0;
		virtual void Execute(const std::string& query) = 0;
		virtual std::vector<std::vector<std::string>> Query(const std::string& query) = 0;
		virtual ~DataBaseInterface() = default;
		DataBaseInterface() = default;
		DataBaseInterface(const DataBaseInterface&) = delete;
		DataBaseInterface& operator=(const DataBaseInterface&) = delete;
		DataBaseInterface(DataBaseInterface&&) = delete;
		DataBaseInterface& operator=(DataBaseInterface&&) = delete;
	};
}