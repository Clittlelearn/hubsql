// SPDX-License-Identifier: MIT
pragma solidity 0.8.19;

/// @notice Test-only ERC20 used to validate OpenHive EVM <-> native UTXO flow.
/// The production Hyperlane token is a separate security and deployment scope.
contract LockableToken {
    string public name;
    string public symbol;
    string public logo;
    uint8 public immutable decimals;
    address public immutable nativeFlowBridge;

    uint256 public totalSupply;
    mapping(address => uint256) public balanceOf;
    mapping(address => mapping(address => uint256)) public allowance;
    mapping(bytes32 => bool) public processedFlowOut;

    event Transfer(address indexed from, address indexed to, uint256 value);
    event Approval(
        address indexed owner,
        address indexed spender,
        uint256 value
    );
    event FlowIn(address indexed account, uint256 amount);
    event FlowOut(
        bytes32 indexed operationId,
        address indexed recipient,
        uint256 amount
    );

    error InvalidBridge();
    error InvalidAccount();
    error InvalidAmount();
    error InsufficientBalance();
    error InsufficientAllowance();
    error OnlyNativeFlowBridge();
    error FlowOutAlreadyProcessed();

    constructor(
        string memory tokenName,
        string memory tokenSymbol,
        uint8 tokenDecimals,
        uint256 initialSupply,
        string memory logoUrl,
        address initialHolder,
        address bridge
    ) {
        if (initialHolder == address(0)) revert InvalidAccount();
        if (bridge == address(0)) revert InvalidBridge();

        name = tokenName;
        symbol = tokenSymbol;
        decimals = tokenDecimals;
        logo = logoUrl;
        nativeFlowBridge = bridge;
        _mint(initialHolder, initialSupply);
    }

    function transfer(address recipient, uint256 amount)
        external
        returns (bool)
    {
        _transfer(msg.sender, recipient, amount);
        return true;
    }

    function approve(address spender, uint256 amount)
        external
        returns (bool)
    {
        if (spender == address(0)) revert InvalidAccount();
        allowance[msg.sender][spender] = amount;
        emit Approval(msg.sender, spender, amount);
        return true;
    }

    function transferFrom(address sender, address recipient, uint256 amount)
        external
        returns (bool)
    {
        uint256 approved = allowance[sender][msg.sender];
        if (approved != type(uint256).max) {
            if (approved < amount) revert InsufficientAllowance();
            unchecked {
                allowance[sender][msg.sender] = approved - amount;
            }
            emit Approval(
                sender,
                msg.sender,
                allowance[sender][msg.sender]
            );
        }
        _transfer(sender, recipient, amount);
        return true;
    }

    /// @notice Burns EVM-layer tokens before OpenHive derives native UTXO.
    function flowIn(uint256 amount) external {
        if (amount == 0) revert InvalidAmount();
        _burn(msg.sender, amount);
        emit FlowIn(msg.sender, amount);
    }

    /// @notice Mints EVM-layer tokens only after consensus validates and
    /// consumes the exact native UTXO plan.
    function flowOut(
        address recipient,
        uint256 amount,
        bytes32 operationId
    ) external {
        if (msg.sender != nativeFlowBridge) {
            revert OnlyNativeFlowBridge();
        }
        if (recipient == address(0)) revert InvalidAccount();
        if (amount == 0 || operationId == bytes32(0)) {
            revert InvalidAmount();
        }
        if (processedFlowOut[operationId]) {
            revert FlowOutAlreadyProcessed();
        }

        processedFlowOut[operationId] = true;
        _mint(recipient, amount);
        emit FlowOut(operationId, recipient, amount);
    }

    function _transfer(address sender, address recipient, uint256 amount)
        internal
    {
        if (sender == address(0) || recipient == address(0)) {
            revert InvalidAccount();
        }
        uint256 balance = balanceOf[sender];
        if (balance < amount) revert InsufficientBalance();
        unchecked {
            balanceOf[sender] = balance - amount;
        }
        balanceOf[recipient] += amount;
        emit Transfer(sender, recipient, amount);
    }

    function _mint(address recipient, uint256 amount) internal {
        if (recipient == address(0)) revert InvalidAccount();
        totalSupply += amount;
        balanceOf[recipient] += amount;
        emit Transfer(address(0), recipient, amount);
    }

    function _burn(address sender, uint256 amount) internal {
        if (sender == address(0)) revert InvalidAccount();
        uint256 balance = balanceOf[sender];
        if (balance < amount) revert InsufficientBalance();
        unchecked {
            balanceOf[sender] = balance - amount;
        }
        totalSupply -= amount;
        emit Transfer(sender, address(0), amount);
    }
}
