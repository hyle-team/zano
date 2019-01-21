import {Component, OnInit, Input} from '@angular/core';
import {Transaction} from '../../models/transaction.model';
import {VariablesService} from "../../services/variables.service";

@Component({
  selector: 'app-transaction-details',
  templateUrl: './transaction-details.component.html',
  styleUrls: ['./transaction-details.component.scss']
})
export class TransactionDetailsComponent implements OnInit {

  @Input() transaction: Transaction;
  cellSizes = {
    1: '25%',
    2: '25%',
    3: '25%',
    4: '25%'
  };

  constructor(private variablesService: VariablesService) {}

  ngOnInit() {}

}
