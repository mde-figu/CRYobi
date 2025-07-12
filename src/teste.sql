CREATE TABLE Usuario (
    ID_Usuario NUMBER PRIMARY KEY,
    Nome VARCHAR2(100) NOT NULL,
    Email VARCHAR2(100) UNIQUE NOT NULL,
    CPF VARCHAR2(14) UNIQUE NOT NULL,
    Senha VARCHAR2(255) NOT NULL,
    Data_Cadastro DATE DEFAULT SYSDATE
);

CREATE TABLE Conta_Bancaria (
    ID_Conta NUMBER PRIMARY KEY,
    ID_Usuario NUMBER,
    Banco VARCHAR2(50) NOT NULL,
    Agencia VARCHAR2(10),
    Numero_Conta VARCHAR2(20) NOT NULL,
    Saldo_Atual NUMBER(15,2) DEFAULT 0.00,
    Tipo_Conta VARCHAR2(20),
    CONSTRAINT fk_conta_usuario FOREIGN KEY (ID_Usuario) REFERENCES Usuario(ID_Usuario)
);

CREATE TABLE Categoria (
    ID_Categoria NUMBER PRIMARY KEY,
    Nome_Categoria VARCHAR2(50) NOT NULL,
    Tipo_Categoria VARCHAR2(20) CHECK (Tipo_Categoria IN ('Receita', 'Despesa_Fixa', 'Despesa_Variavel')),
    ID_Usuario NUMBER,
    CONSTRAINT fk_categoria_usuario FOREIGN KEY (ID_Usuario) REFERENCES Usuario(ID_Usuario)
);

CREATE TABLE Transacao (
    ID_Transacao NUMBER PRIMARY KEY,
    ID_Conta NUMBER,
    ID_Categoria NUMBER,
    Valor NUMBER(15,2) NOT NULL,
    
    Data_Transacao DATE DEFAULT SYSDATE,
    Descricao VARCHAR2(255),
    Tipo VARCHAR2(10) CHECK (Tipo IN ('Entrada', 'Saida')),
    CONSTRAINT fk_transacao_conta FOREIGN KEY (ID_Conta) REFERENCES Conta_Bancaria(ID_Conta),
    CONSTRAINT fk_transacao_categoria FOREIGN KEY (ID_Categoria) REFERENCES Categoria(ID_Categoria)
);

CREATE TABLE Endereco (
    ID_Endereco NUMBER PRIMARY KEY,
    ID_Usuario NUMBER,
    Logradouro VARCHAR2(100),
    Numero VARCHAR2(10),
    Complemento VARCHAR2(50),
    Bairro VARCHAR2(50),
    Cidade VARCHAR2(50),
    Estado VARCHAR2(2),
    CEP VARCHAR2(9),
    CONSTRAINT fk_endereco_usuario FOREIGN KEY (ID_Usuario) REFERENCES Usuario(ID_Usuario)
);

CREATE TABLE Contato (
    ID_Contato NUMBER PRIMARY KEY,
    ID_Usuario NUMBER,
    Tipo_Contato VARCHAR2(20) CHECK (Tipo_Contato IN ('Telefone', 'Email', 'WhatsApp')),
    Valor_Contato VARCHAR2(100) NOT NULL,
    CONSTRAINT fk_contato_usuario FOREIGN KEY (ID_Usuario) REFERENCES Usuario(ID_Usuario)
);

CREATE TABLE Relatorio (
    ID_Relatorio NUMBER PRIMARY KEY,
    ID_Usuario NUMBER,
    Data_Geracao DATE DEFAULT SYSDATE,
    Tipo_Relatorio VARCHAR2(50),
    Conteudo CLOB,
    CONSTRAINT fk_relatorio_usuario FOREIGN KEY (ID_Usuario) REFERENCES Usuario(ID_Usuario)
);

CREATE TABLE Permissao (
    ID_Permissao NUMBER PRIMARY KEY,
    ID_Usuario NUMBER,
    ID_Conta NUMBER,
    Nivel_Acesso VARCHAR2(20) CHECK (Nivel_Acesso IN ('Admin', 'Editor', 'Visualizador')),
    CONSTRAINT fk_permissao_usuario FOREIGN KEY (ID_Usuario) REFERENCES Usuario(ID_Usuario),
    CONSTRAINT fk_permissao_conta FOREIGN KEY (ID_Conta) REFERENCES Conta_Bancaria(ID_Conta)
);

-- Índices para melhorar performance
CREATE INDEX idx_transacao_data ON Transacao(Data_Transacao);
CREATE INDEX idx_conta_usuario ON Conta_Bancaria(ID_Usuario);