#pragma once
#include <vector>
#include <string>
namespace RbsLib::DataBase
{
/*
* ���ݿ�ӿ�һ��Ӧ���ɿ��������ڶദ��Ҫ����ʱ����ʹ������ָ���������
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