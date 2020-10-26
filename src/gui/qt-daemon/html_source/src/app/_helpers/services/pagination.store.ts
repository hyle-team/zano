import {Injectable} from '@angular/core';
import {Observable, BehaviorSubject} from 'rxjs';
import {VariablesService} from './variables.service';
import * as _ from 'lodash';

export interface Pages {
   page: number;
  offset: number;
}
@Injectable({
  providedIn: 'root'
})
export class PaginationStore {
  constructor(
    private variablesService: VariablesService
  ) {
  }
  private subject = new BehaviorSubject<Pages[]>(null);
  pages$: Observable<Pages[]> = this.subject.asObservable();

  isForward(pages, currentPage) {
    const max = _.maxBy(pages, 'page');
    return !max || max.page < currentPage || max.page === currentPage;
  }
  setPage(pageNumber: number, offset: number, erase: boolean) {
    let newPages: Pages[] = [];
    if (!erase) {
      const pages = this.subject.getValue();
      if (pages && pages.length) {
        newPages = pages.slice(0);
      }
      newPages.push({page: pageNumber, offset});
    } else { // clean from values on change wallet
      newPages = undefined;
    }
    this.subject.next(newPages);
  }

  get value() {
    return this.subject.value;
  }

}
