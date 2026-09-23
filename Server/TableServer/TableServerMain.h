#pragma once
class TableServerMain
{
public:
	TableServerMain();
	~TableServerMain();

	BOOL StartServer();

private:
	void DestroyServerResources();

private:
	BOOL InitServerResources();
	
};

